#include "SerialWorker.h"

#include "AppLogging.h"
#include "ControlCommandEncoder.h"

#include <algorithm>
#include <QIODevice>
#include <QMetaObject>

namespace {
constexpr int kWorkerControlHeartbeatPeriodMs = 50;
constexpr int kWorkerControlLeaseRenewPeriodMs = 250;
constexpr quint16 kWorkerControlLeaseMs = 2000;
constexpr int kTypedLiveEmitBatchSize = 512;
constexpr int kTypedHandshakeWatchdogIntervalMs = 250;
constexpr int kTypedHandshakeTimeoutMs = 3500;
}

SerialWorker::SerialWorker(QObject* parent)
    : QObject(parent),
      m_recorder(this) {}

void SerialWorker::start(const QString& portName) {
    stop();

    m_typedBytesSinceOpen = 0;
    m_typedCapabilitySeenSinceOpen = false;

    m_serial = new QSerialPort(this);
    m_serial->setPortName(portName);
    m_serial->setBaudRate(921600);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        qCWarning(logTransport).noquote() << "Serial open failed" << portName << m_serial->errorString();
        emit stateChanged(false, QStringLiteral("연결 실패: %1").arg(m_serial->errorString()));
        m_serial->deleteLater();
        m_serial = nullptr;
        return;
    }

    m_serial->setDataTerminalReady(true);
    m_serial->setRequestToSend(true);
    m_parser.reset();
    m_typedParser.reset();
    connect(m_serial, &QSerialPort::readyRead, this, &SerialWorker::onReadyRead);
    connect(m_serial, &QSerialPort::bytesWritten, this, &SerialWorker::onBytesWritten);
    connect(m_serial, &QSerialPort::errorOccurred, this, [this, portName](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError || !m_serial) return;

        const QString message = QStringLiteral("%1: %2").arg(portName, m_serial->errorString());
        qCWarning(logTransport).noquote() << "Serial runtime error" << int(error) << message;
        emit errorOccurred(QStringLiteral("Serial port error: %1").arg(message));

        if (error == QSerialPort::ResourceError ||
            error == QSerialPort::PermissionError ||
            error == QSerialPort::DeviceNotFoundError) {
            QMetaObject::invokeMethod(this, [this, message]() {
                closeSerialPortForRecovery(QStringLiteral("serial runtime error: %1").arg(message));
                emit stateChanged(false, QStringLiteral("Serial port closed after error"));
            }, Qt::QueuedConnection);
        }
    });
    if (m_transportMode == TransportMode::TypedEvidence) {
        startTypedHandshakeWatchdog();
    }
    const QString modeText = (m_transportMode == TransportMode::TypedEvidence)
        ? QStringLiteral("typed evidence")
        : QStringLiteral("legacy 20B");
    qCInfo(logTransport).noquote() << "Serial connected" << portName << modeText;
    emit stateChanged(true, QStringLiteral("연결됨: %1 · %2").arg(portName, modeText));
}

void SerialWorker::stop() {
    stopControlCycle();
    stopTypedHandshakeWatchdog();
    const bool wasLogging = m_recorder.isActive();
    const QString lastPath = m_recorder.currentPath();
    m_recorder.stop();
    if (!m_recorder.lastError().trimmed().isEmpty()) {
        emit errorOccurred(QStringLiteral("로그 종료 후처리 경고: %1").arg(m_recorder.lastError()));
    }
    if (wasLogging) {
        emit loggingProgress(m_recorder.bytesWritten(), m_recorder.frameCount());
        emit loggingStateChanged(false, lastPath);
    }
    const bool wasTypedStorage = m_typedStorage.isActive();
    const QString typedPath = m_typedStorage.paths().sessionDir;
    if (wasTypedStorage) {
        QString error;
        if (!m_typedStorage.finalizeTypedSession(&error)) {
            emit errorOccurred(QStringLiteral("Typed storage finalize failed: %1").arg(error));
        }
        emit typedStorageProgress(m_typedStorage.bytesWritten(), m_typedStorage.recordCount());
        emit typedStorageStateChanged(false, typedPath);
    }
    closeSerialPortForRecovery(QStringLiteral("operator disconnect"));
    clearHostTxQueue(QStringLiteral("serial disconnected"));
    emit stateChanged(false, QStringLiteral("연결 해제"));
}

bool SerialWorker::setTypedStorage(bool enable, const QString& sessionDir, const QJsonObject& metadata) {
    if (enable) {
        if (m_transportMode != TransportMode::TypedEvidence) {
            emit errorOccurred(QStringLiteral("Typed storage requires TypedEvidence transport mode."));
            return false;
        }
        if (m_typedStorage.isActive()) {
            emit errorOccurred(QStringLiteral("Typed storage is already active."));
            return false;
        }

        QString error;
        if (!m_typedStorage.startTypedSession(sessionDir, metadata, &error)) {
            emit errorOccurred(QStringLiteral("Typed storage start failed: %1").arg(error));
            return false;
        }
        m_lastReportedFrameCount = 0;
        m_logProgressTimer.restart();
        emit typedStorageProgress(m_typedStorage.bytesWritten(), m_typedStorage.recordCount());
        emit typedStorageStateChanged(true, m_typedStorage.paths().sessionDir);
        return true;
    }

    if (!m_typedStorage.isActive()) {
        emit typedStorageStateChanged(false, sessionDir);
        return true;
    }

    const QString path = m_typedStorage.paths().sessionDir;
    QString error;
    if (!m_typedStorage.finalizeTypedSession(&error)) {
        emit errorOccurred(QStringLiteral("Typed storage finalize failed: %1").arg(error));
        return false;
    }
    emit typedStorageProgress(m_typedStorage.bytesWritten(), m_typedStorage.recordCount());
    emit typedStorageStateChanged(false, path);
    return true;
}

void SerialWorker::sendHostFrame(const QByteArray& frame, const QString& summary) {
    if (frame.isEmpty()) {
        emit hostFrameWriteResult(false, QStringLiteral("empty host frame"), 0);
        return;
    }
    if (!m_serial || !m_serial->isOpen()) {
        emit hostFrameWriteResult(false, summary.isEmpty() ? QStringLiteral("serial not connected") : summary, 0);
        return;
    }
    if (!m_serial->isWritable()) {
        emit hostFrameWriteResult(false, summary.isEmpty() ? QStringLiteral("serial is not writable") : summary, 0);
        return;
    }

    QString error;
    const QString normalizedSummary = summary.isEmpty() ? QStringLiteral("host frame") : summary;
    if (!m_hostTxQueue.enqueue(frame, normalizedSummary, &error)) {
        emit hostFrameWriteResult(false, error, 0);
        emitHostTxQueueStatus();
        return;
    }

    emitHostTxQueueStatus();
    drainHostTxQueue();
}

void SerialWorker::startControlCycle(int signedCommand,
                                     int rpm,
                                     double steeringDeg,
                                     quint8 motorMode,
                                     quint8 drivingMode,
                                     quint8 bus,
                                     int periodMs,
                                     int frameGapMs) {
    updateControlCycle(signedCommand, rpm, steeringDeg, motorMode, drivingMode, bus);
    m_controlSlewLimiter.reset();
    m_controlSlewLimiter.setTarget({
        m_controlCycleSignedCommand,
        m_controlCycleRpm,
        m_controlCycleSteeringDeg,
    });
    m_controlCyclePeriodMs = std::clamp(periodMs, 5, 1000);
    m_controlCycleFrameGapMs = std::clamp(frameGapMs, 0, 20);
    m_controlCycleEnabled = true;
    m_controlCycleClock.restart();
    m_controlCycleLastHeartbeatMs = -1;
    m_controlCycleLastLeaseRenewMs = -1;

    if (m_controlCycleTimerId != 0) killTimer(m_controlCycleTimerId);
    m_controlCycleTimerId = startTimer(m_controlCyclePeriodMs, Qt::PreciseTimer);
    beginControlCycle();
}

void SerialWorker::updateControlCycle(int signedCommand,
                                      int rpm,
                                      double steeringDeg,
                                      quint8 motorMode,
                                      quint8 drivingMode,
                                      quint8 bus) {
    m_controlCycleSignedCommand = std::clamp(signedCommand, -10000, 10000);
    m_controlCycleRpm = std::clamp(rpm, 0, 10000);
    m_controlCycleSteeringDeg = std::clamp(steeringDeg, -90.0, 90.0);
    m_controlCycleMotorMode = motorMode == 2 ? 2 : 1;
    m_controlCycleDrivingMode = drivingMode;
    m_controlCycleBus = bus;
    m_controlSlewLimiter.setTarget({
        m_controlCycleSignedCommand,
        m_controlCycleRpm,
        m_controlCycleSteeringDeg,
    });
}

void SerialWorker::stopControlCycle() {
    m_controlCycleEnabled = false;
    m_controlCycleSending = false;
    m_controlCycleSignedCommand = 0;
    m_controlCycleRpm = 0;
    m_controlCycleSteeringDeg = 0.0;
    m_controlSlewLimiter.reset();
    if (m_controlCycleTimerId != 0) {
        killTimer(m_controlCycleTimerId);
        m_controlCycleTimerId = 0;
    }
    if (m_controlCycleGapTimerId != 0) {
        killTimer(m_controlCycleGapTimerId);
        m_controlCycleGapTimerId = 0;
    }
    m_controlCyclePendingFrames.clear();
    m_controlCyclePendingSummaries.clear();
    m_controlCyclePendingIndex = 0;
}

void SerialWorker::sendControlCycleBurstOnce(int signedCommand,
                                             int rpm,
                                             double steeringDeg,
                                             quint8 motorMode,
                                             quint8 drivingMode,
                                             quint8 bus,
                                             const QString& reason) {
    updateControlCycle(signedCommand, rpm, steeringDeg, motorMode, drivingMode, bus);
    sendControlCycleBurst(reason.isEmpty() ? QStringLiteral("worker one-shot") : reason);
}

void SerialWorker::setLogging(bool enable, const QString& binPath, const QString& metaPath, const QString& rulesSnapshotPath, const QString& rulesSourcePath) {
    if (m_transportMode == TransportMode::TypedEvidence) {
        if (enable) {
            emit errorOccurred(QStringLiteral("Typed evidence 모드의 저장은 아직 안전 게이트 전입니다. 현재 턴은 수신/파싱 확인만 지원합니다."));
        }
        return;
    }

    if (!enable) {
        const bool wasActive = m_recorder.isActive();
        const QString lastPath = m_recorder.currentPath();
        m_recorder.stop();
        if (!m_recorder.lastError().trimmed().isEmpty()) {
            emit errorOccurred(QStringLiteral("로그 종료 후처리 경고: %1").arg(m_recorder.lastError()));
        }
        if (wasActive) {
            emit loggingProgress(m_recorder.bytesWritten(), m_recorder.frameCount());
            emit loggingStateChanged(false, lastPath);
        }
        return;
    }
    QString err;
    if (!m_recorder.start(binPath, metaPath, rulesSnapshotPath, rulesSourcePath, &err)) {
        emit errorOccurred(QStringLiteral("로그 시작 실패: %1").arg(err));
    } else {
        m_lastReportedFrameCount = 0;
        m_logProgressTimer.restart();
        emit loggingProgress(m_recorder.bytesWritten(), m_recorder.frameCount());
        emit loggingStateChanged(true, m_recorder.currentPath());
    }
}

void SerialWorker::onReadyRead() {
    if (!m_serial) return;
    processIncomingBytes(m_serial->readAll());
}

void SerialWorker::onBytesWritten(qint64 bytes) {
    Q_UNUSED(bytes);
    drainHostTxQueue();
}

void SerialWorker::timerEvent(QTimerEvent* event) {
    if (event->timerId() == m_typedHandshakeTimerId) {
        if (m_typedCapabilitySeenSinceOpen) {
            stopTypedHandshakeWatchdog();
            return;
        }
        if (m_typedHandshakeClock.isValid() && m_typedHandshakeClock.elapsed() >= kTypedHandshakeTimeoutMs) {
            const quint64 frames = m_typedParser.counters().frames;
            const QString reason = QStringLiteral("typed handshake timeout: no CAPABILITY after %1ms, bytes=%2, frames=%3")
                .arg(m_typedHandshakeClock.elapsed())
                .arg(m_typedBytesSinceOpen)
                .arg(frames);
            qCWarning(logTransport).noquote() << reason;
            emit errorOccurred(reason);
            closeSerialPortForRecovery(reason);
            clearHostTxQueue(reason);
            emit stateChanged(false, QStringLiteral("Typed evidence handshake timeout; port closed"));
            return;
        }
        return;
    }
    if (event->timerId() == m_controlCycleTimerId) {
        beginControlCycle();
        return;
    }
    if (event->timerId() == m_controlCycleGapTimerId) {
        if (m_controlCycleGapTimerId != 0) {
            killTimer(m_controlCycleGapTimerId);
            m_controlCycleGapTimerId = 0;
        }
        sendNextControlCycleFrame();
        return;
    }
    QObject::timerEvent(event);
}

void SerialWorker::setTransportMode(TransportMode mode) {
    if (m_transportMode == mode) return;
    m_transportMode = mode;
    m_parser.reset();
    m_typedParser.reset();
    m_typedBytesSinceOpen = 0;
    m_typedCapabilitySeenSinceOpen = false;
    if (m_serial && m_serial->isOpen()) {
        if (m_transportMode == TransportMode::TypedEvidence) {
            startTypedHandshakeWatchdog();
        } else {
            stopTypedHandshakeWatchdog();
        }
    }
}

void SerialWorker::ingestBytesForTest(const QByteArray& bytes) {
    processIncomingBytes(bytes);
}

void SerialWorker::processIncomingBytes(const QByteArray& bytes) {
    if (bytes.isEmpty()) return;
    if (m_transportMode == TransportMode::TypedEvidence) {
        m_typedBytesSinceOpen += quint64(bytes.size());
    }
    if (m_transportMode == TransportMode::TypedEvidence) {
        processTypedBytes(bytes);
        return;
    }
    processLegacyBytes(bytes);
}

void SerialWorker::processLegacyBytes(const QByteArray& bytes) {
    m_parser.append(bytes);

    FrameRecordList batch;
    batch.reserve(192);
    auto flushBatch = [this, &batch]() {
        if (batch.isEmpty()) return;
        emit framesReceived(batch);
        batch.clear();
        batch.reserve(192);
    };

    while (true) {
        auto rec = m_parser.takeOne();
        if (!rec) break;
        if (rec->isStats) {
            flushBatch();
            if (m_recorder.isActive()) m_recorder.flushPending(false);
            emit statsReceived(rec->stats);
        } else {
            batch.push_back(rec->frame);
            if (m_recorder.isActive()) {
                m_recorder.append20(rec->frame.raw20);
            }
            if (batch.size() >= 192) flushBatch();
        }
    }

    flushBatch();
    if (m_recorder.isActive()) {
        m_recorder.flushPending(false);
        const quint64 frames = m_recorder.frameCount();
        const bool byCount = (frames >= m_lastReportedFrameCount + 1536);
        const bool byTime = (!m_logProgressTimer.isValid() || m_logProgressTimer.elapsed() >= 1000);
        if (byCount || byTime) {
            emit loggingProgress(m_recorder.bytesWritten(), frames);
            m_lastReportedFrameCount = frames;
            m_logProgressTimer.restart();
        }
    }
}

void SerialWorker::processTypedBytes(const QByteArray& bytes) {
    const TypedTransportParser::Counters countersBefore = m_typedParser.counters();
    m_typedParser.append(bytes);

    TypedRecordList batch;
    batch.reserve(kTypedLiveEmitBatchSize);
    while (true) {
        auto record = m_typedParser.takeOne();
        if (!record) break;
        if (!m_typedCapabilitySeenSinceOpen && record->isType(TypedRecordType::Capability)) {
            m_typedCapabilitySeenSinceOpen = true;
            qCInfo(logTransport).noquote()
                << "Typed CAPABILITY received after"
                << (m_typedHandshakeClock.isValid() ? m_typedHandshakeClock.elapsed() : -1)
                << "ms"
                << "bytes" << m_typedBytesSinceOpen;
            stopTypedHandshakeWatchdog();
        }
        if (m_typedStorage.isActive()) {
            QString error;
            if (!m_typedStorage.appendTypedRecord(*record, &error)) {
                emit errorOccurred(QStringLiteral("Typed storage append failed: %1").arg(error));
            }
        }
        batch.push_back(*record);
        if (batch.size() >= kTypedLiveEmitBatchSize) {
            emit typedRecordsReceived(batch);
            batch.clear();
            batch.reserve(kTypedLiveEmitBatchSize);
        }
    }

    if (!batch.isEmpty()) emit typedRecordsReceived(batch);
    if (m_typedStorage.isActive()) {
        const quint64 records = m_typedStorage.recordCount();
        const bool byCount = (records >= m_lastReportedFrameCount + 1536);
        const bool byTime = (!m_logProgressTimer.isValid() || m_logProgressTimer.elapsed() >= 1000);
        if (byCount || byTime) {
            emit typedStorageProgress(m_typedStorage.bytesWritten(), records);
            m_lastReportedFrameCount = records;
            m_logProgressTimer.restart();
        }
    }
    if (typedCountersChanged(countersBefore)) emitTypedStatus();
}

void SerialWorker::emitTypedStatus() {
    if (m_typedStatusTimer.isValid() && m_typedStatusTimer.elapsed() < m_typedStatusMinIntervalMs) return;
    m_typedStatusTimer.restart();
    const auto& counters = m_typedParser.counters();
    emit typedTransportStatusChanged(counters.frames,
                                     counters.bytesDropped,
                                     counters.crcFailures,
                                     counters.lengthFailures,
                                     counters.versionWarnings,
                                     counters.seqGaps);
}

bool SerialWorker::typedCountersChanged(const TypedTransportParser::Counters& before) const {
    const auto& after = m_typedParser.counters();
    return after.frames != before.frames
        || after.bytesDropped != before.bytesDropped
        || after.crcFailures != before.crcFailures
        || after.lengthFailures != before.lengthFailures
        || after.versionWarnings != before.versionWarnings
        || after.seqGaps != before.seqGaps;
}

void SerialWorker::startTypedHandshakeWatchdog() {
    stopTypedHandshakeWatchdog();
    m_typedCapabilitySeenSinceOpen = false;
    m_typedHandshakeClock.restart();
    m_typedHandshakeTimerId = startTimer(kTypedHandshakeWatchdogIntervalMs, Qt::CoarseTimer);
}

void SerialWorker::stopTypedHandshakeWatchdog() {
    if (m_typedHandshakeTimerId != 0) {
        killTimer(m_typedHandshakeTimerId);
        m_typedHandshakeTimerId = 0;
    }
    m_typedHandshakeClock.invalidate();
}

void SerialWorker::closeSerialPortForRecovery(const QString& reason) {
    stopTypedHandshakeWatchdog();
    stopControlCycle();
    if (!m_serial) return;

    const QString portName = m_serial->portName();
    qCInfo(logTransport).noquote() << "Closing serial port" << portName << reason;
    disconnect(m_serial, nullptr, this, nullptr);
    if (m_serial->isOpen()) {
        m_serial->clear(QSerialPort::AllDirections);
        m_serial->setRequestToSend(false);
        m_serial->setDataTerminalReady(false);
        m_serial->close();
    }
    m_serial->deleteLater();
    m_serial = nullptr;
    m_typedBytesSinceOpen = 0;
    m_typedCapabilitySeenSinceOpen = false;
}

void SerialWorker::drainHostTxQueue() {
    if (!m_serial || !m_serial->isOpen() || !m_serial->isWritable()) return;

    while (m_hostTxQueue.hasPending() && m_serial->bytesToWrite() < m_hostTxMaxInFlightBytes) {
        const auto item = m_hostTxQueue.dequeue();
        const qint64 written = m_serial->write(item.frame);
        const QString summary = item.summary.isEmpty() ? QStringLiteral("host frame") : item.summary;

        if (written != item.frame.size()) {
            emit hostFrameWriteResult(false,
                                      written < 0 ? QStringLiteral("%1 write failed: %2").arg(summary, m_serial->errorString())
                                                  : QStringLiteral("%1 partial write: %2/%3 bytes").arg(summary).arg(written).arg(item.frame.size()),
                                      written > 0 ? quint64(written) : 0);
            emitHostTxQueueStatus();
            return;
        }

        m_hostTxQueue.markWritten();
        emit hostFrameWriteResult(true, summary, quint64(written));
    }

    emitHostTxQueueStatus();
}

void SerialWorker::clearHostTxQueue(const QString& reason) {
    if (!m_hostTxQueue.hasPending()) {
        emitHostTxQueueStatus();
        return;
    }

    const quint64 dropped = quint64(m_hostTxQueue.queuedFrames());
    m_hostTxQueue.clear();
    emit errorOccurred(QStringLiteral("Host TX queue cleared: %1 · %2 frame(s)").arg(reason).arg(dropped));
    emitHostTxQueueStatus();
}

void SerialWorker::emitHostTxQueueStatus() {
    emit hostTxQueueChanged(quint64(m_hostTxQueue.queuedFrames()),
                            quint64(m_hostTxQueue.queuedBytes()),
                            m_hostTxQueue.enqueuedFrames(),
                            m_hostTxQueue.writtenFrames(),
                            m_hostTxQueue.droppedFrames());
}

quint32 SerialWorker::nextControlCycleCommandId() {
    if (m_controlCycleCommandCounter < 0x80000000u) {
        m_controlCycleCommandCounter = 0x80000000u;
    }
    return ++m_controlCycleCommandCounter;
}

void SerialWorker::maintainControlCycleSession() {
    if (!m_controlCycleClock.isValid()) m_controlCycleClock.restart();
    const qint64 nowMs = m_controlCycleClock.elapsed();
    if (m_controlCycleLastHeartbeatMs < 0 || nowMs - m_controlCycleLastHeartbeatMs >= kWorkerControlHeartbeatPeriodMs) {
        const quint32 commandId = nextControlCycleCommandId();
        const quint32 hostMonoMs = quint32(nowMs & 0xFFFFFFFFLL);
        const QByteArray frame = CanMonitorControl::ControlCommandEncoder::buildHostHeartbeat(commandId, hostMonoMs);
        sendHostFrame(frame, QStringLiteral("#%1 worker HOST_HEARTBEAT").arg(commandId));
        m_controlCycleLastHeartbeatMs = nowMs;
    }
    if (m_controlCycleLastLeaseRenewMs < 0 || nowMs - m_controlCycleLastLeaseRenewMs >= kWorkerControlLeaseRenewPeriodMs) {
        const quint32 commandId = nextControlCycleCommandId();
        const QByteArray frame = CanMonitorControl::ControlCommandEncoder::buildHostControlSession(
            commandId,
            CanMonitorControl::kControlSessionRenewLease,
            CanMonitorControl::kControlSessionAnyBus,
            kWorkerControlLeaseMs);
        sendHostFrame(frame, QStringLiteral("#%1 worker HOST_CONTROL_SESSION RENEW_LEASE").arg(commandId));
        m_controlCycleLastLeaseRenewMs = nowMs;
    }
}

void SerialWorker::beginControlCycle() {
    if (!m_controlCycleEnabled) return;
    maintainControlCycleSession();
    if (m_controlCycleSending) {
        emit errorOccurred(QStringLiteral("Control cycle overrun: previous burst is still being paced"));
        return;
    }
    sendControlCycleBurst(QStringLiteral("worker 20ms cycle"));
}

CanMonitorControl::ControlSlewState SerialWorker::advanceControlCycleRamp() {
    return m_controlSlewLimiter.step();
}

void SerialWorker::sendControlCycleBurst(const QString& reason) {
    const auto commandState = advanceControlCycleRamp();
    const auto frames = CanMonitorControl::ControlCommandEncoder::makeControlBurst(
        commandState.signedCommand,
        commandState.rpm,
        commandState.steeringDeg,
        m_controlCycleMotorMode,
        m_controlCycleDrivingMode,
        m_controlCycleAliveCounter++,
        m_controlCycleBus);

    m_controlCyclePendingFrames.clear();
    m_controlCyclePendingSummaries.clear();
    m_controlCyclePendingFrames.reserve(frames.size());
    m_controlCyclePendingSummaries.reserve(frames.size());
    for (const auto& frame : frames) {
        const quint32 commandId = nextControlCycleCommandId();
        m_controlCyclePendingFrames.push_back(CanMonitorControl::ControlCommandEncoder::buildHostCanTxRequest(commandId, frame));
        m_controlCyclePendingSummaries.push_back(QStringLiteral("#%1 %2 %3")
            .arg(commandId)
            .arg(reason)
            .arg(CanMonitorControl::ControlCommandEncoder::frameSummary(frame)));
    }

    m_controlCyclePendingIndex = 0;
    m_controlCycleSending = true;
    sendNextControlCycleFrame();
}

void SerialWorker::sendNextControlCycleFrame() {
    if (m_controlCyclePendingIndex >= m_controlCyclePendingFrames.size()) {
        m_controlCycleSending = false;
        m_controlCyclePendingFrames.clear();
        m_controlCyclePendingSummaries.clear();
        m_controlCyclePendingIndex = 0;
        return;
    }

    const int index = m_controlCyclePendingIndex++;
    sendHostFrame(m_controlCyclePendingFrames.at(index), m_controlCyclePendingSummaries.value(index));
    if (m_controlCyclePendingIndex >= m_controlCyclePendingFrames.size()) {
        m_controlCycleSending = false;
        return;
    }

    if (m_controlCycleFrameGapMs <= 0) {
        sendNextControlCycleFrame();
        return;
    }
    if (m_controlCycleGapTimerId != 0) killTimer(m_controlCycleGapTimerId);
    m_controlCycleGapTimerId = startTimer(m_controlCycleFrameGapMs, Qt::PreciseTimer);
}
