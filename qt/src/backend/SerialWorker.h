#pragma once

#include "CanTypes.h"
#include "PacketParser.h"
#include "Recorder.h"
#include "StorageRuntime.h"
#include "TypedTransportParser.h"
#include "control/ControlSlewLimiter.h"
#include "transport/HostTxQueue.h"

#include <QJsonObject>
#include <QObject>
#include <QSerialPort>
#include <QElapsedTimer>
#include <QStringList>
#include <QTimerEvent>
#include <QVector>

class SerialWorker : public QObject {
    Q_OBJECT
public:
    enum class TransportMode {
        Legacy20 = 0,
        TypedEvidence = 1
    };
    Q_ENUM(TransportMode)

    explicit SerialWorker(QObject* parent = nullptr);
    TransportMode transportMode() const { return m_transportMode; }
    void ingestBytesForTest(const QByteArray& bytes);

public slots:
    void start(const QString& portName);
    void stop();
    void setTransportMode(TransportMode mode);
    void setLogging(bool enable, const QString& binPath, const QString& metaPath, const QString& rulesSnapshotPath, const QString& rulesSourcePath);
    bool setTypedStorage(bool enable, const QString& sessionDir, const QJsonObject& metadata);
    void sendHostFrame(const QByteArray& frame, const QString& summary);
    void startControlCycle(int signedCommand, int rpm, double steeringDeg, quint8 motorMode, quint8 drivingMode, quint8 bus, int periodMs, int frameGapMs);
    void updateControlCycle(int signedCommand, int rpm, double steeringDeg, quint8 motorMode, quint8 drivingMode, quint8 bus);
    void stopControlCycle();
    void sendControlCycleBurstOnce(int signedCommand, int rpm, double steeringDeg, quint8 motorMode, quint8 drivingMode, quint8 bus, const QString& reason);

signals:
    void stateChanged(bool connected, const QString& message);
    void errorOccurred(const QString& message);
    void framesReceived(const FrameRecordList& frames);
    void statsReceived(const StatsRecord& st);
    void typedRecordsReceived(const TypedRecordList& records);
    void typedTransportStatusChanged(quint64 frames, quint64 bytesDropped, quint64 crcFailures, quint64 lengthFailures, quint64 versionWarnings, quint64 seqGaps);
    void typedStorageStateChanged(bool active, const QString& path);
    void typedStorageProgress(quint64 bytesWritten, quint64 recordCount);
    void hostFrameWriteResult(bool ok, const QString& summary, quint64 bytesWritten);
    void loggingStateChanged(bool active, const QString& path);
    void loggingProgress(quint64 bytesWritten, quint64 frameCount);
    void hostTxQueueChanged(quint64 queuedFrames, quint64 queuedBytes, quint64 enqueuedFrames, quint64 writtenFrames, quint64 droppedFrames);

private slots:
    void onReadyRead();
    void onBytesWritten(qint64 bytes);

private:
    void timerEvent(QTimerEvent* event) override;
    void processIncomingBytes(const QByteArray& bytes);
    void processLegacyBytes(const QByteArray& bytes);
    void processTypedBytes(const QByteArray& bytes);
    void emitTypedStatus();
    bool typedCountersChanged(const TypedTransportParser::Counters& before) const;
    void startTypedHandshakeWatchdog();
    void stopTypedHandshakeWatchdog();
    void closeSerialPortForRecovery(const QString& reason);
    void drainHostTxQueue();
    void clearHostTxQueue(const QString& reason);
    void emitHostTxQueueStatus();
    void beginControlCycle();
    CanMonitorControl::ControlSlewState advanceControlCycleRamp();
    void sendNextControlCycleFrame();
    void sendControlCycleBurst(const QString& reason);
    void maintainControlCycleSession();
    quint32 nextControlCycleCommandId();

    QSerialPort* m_serial = nullptr;
    PacketParser m_parser;
    TypedTransportParser m_typedParser;
    Recorder m_recorder;
    StorageRuntime m_typedStorage;
    CanMonitorTransport::HostTxQueue m_hostTxQueue;
    QElapsedTimer m_logProgressTimer;
    QElapsedTimer m_typedStatusTimer;
    QElapsedTimer m_typedHandshakeClock;
    quint64 m_lastReportedFrameCount = 0;
    quint64 m_typedBytesSinceOpen = 0;
    int m_typedStatusMinIntervalMs = 100;
    int m_typedHandshakeTimerId = 0;
    qsizetype m_hostTxMaxInFlightBytes = 8192;
    bool m_typedCapabilitySeenSinceOpen = false;
    TransportMode m_transportMode = TransportMode::Legacy20;

    bool m_controlCycleEnabled = false;
    bool m_controlCycleSending = false;
    int m_controlCycleTimerId = 0;
    int m_controlCycleGapTimerId = 0;
    int m_controlCyclePeriodMs = 20;
    int m_controlCycleFrameGapMs = 2;
    int m_controlCycleSignedCommand = 0;
    int m_controlCycleRpm = 0;
    double m_controlCycleSteeringDeg = 0.0;
    CanMonitorControl::ControlSlewLimiter m_controlSlewLimiter;
    quint8 m_controlCycleMotorMode = 1;
    quint8 m_controlCycleDrivingMode = 1;
    quint8 m_controlCycleBus = 0;
    quint8 m_controlCycleAliveCounter = 0;
    quint32 m_controlCycleCommandCounter = 0x80000000u;
    qint64 m_controlCycleLastHeartbeatMs = -1;
    qint64 m_controlCycleLastLeaseRenewMs = -1;
    QElapsedTimer m_controlCycleClock;
    QVector<QByteArray> m_controlCyclePendingFrames;
    QStringList m_controlCyclePendingSummaries;
    int m_controlCyclePendingIndex = 0;
};
