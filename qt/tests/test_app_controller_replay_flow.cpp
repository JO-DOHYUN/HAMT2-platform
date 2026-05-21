#include "AppController.h"
#include "TypedTransportParser.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <algorithm>

namespace {

quint8 crc8Atm(const QByteArray& bytes) {
    quint8 crc = 0x00;
    for (const char byte : bytes) {
        crc ^= quint8(byte);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x80) crc = quint8((crc << 1) ^ 0x07);
            else crc = quint8(crc << 1);
        }
    }
    return crc;
}

QByteArray framePacket(quint32 tus, quint32 canId, quint8 bus, quint8 seq, const QByteArray& payload) {
    QByteArray packet(20, Qt::Uninitialized);
    packet[0] = char(tus & 0xFF);
    packet[1] = char((tus >> 8) & 0xFF);
    packet[2] = char((tus >> 16) & 0xFF);
    packet[3] = char((tus >> 24) & 0xFF);
    packet[4] = char(canId & 0xFF);
    packet[5] = char((canId >> 8) & 0xFF);
    packet[6] = char((canId >> 16) & 0xFF);
    packet[7] = char((canId >> 24) & 0x1F);
    packet[8] = char(payload.size() & 0x0F);
    for (int index = 0; index < 8; ++index) {
        packet[9 + index] = index < payload.size() ? payload.at(index) : char(0);
    }
    packet[17] = char(bus & 0x03);
    packet[18] = char(seq);
    packet[19] = char(crc8Atm(packet.left(19)));
    return packet;
}

void appendU16Le(QByteArray& out, quint16 value) {
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
}

void appendU32Le(QByteArray& out, quint32 value) {
    for (int byte = 0; byte < 4; ++byte) out.append(char((value >> (byte * 8)) & 0xFF));
}

void appendU64Le(QByteArray& out, quint64 value) {
    for (int byte = 0; byte < 8; ++byte) out.append(char((value >> (byte * 8)) & 0xFF));
}

QByteArray typedFrame(TypedRecordType type, quint16 seq, const QByteArray& payload) {
    QByteArray frame;
    frame.reserve(int(kTypedTransportFrameOverhead + payload.size()));
    frame.append(char(kTypedTransportSof0));
    frame.append(char(kTypedTransportSof1));
    frame.append(char(kTypedTransportVersion));
    frame.append(char(static_cast<quint8>(type)));
    frame.append(char(0));
    appendU16Le(frame, seq);
    appendU16Le(frame, quint16(payload.size()));
    frame.append(payload);
    const auto* crcStart = reinterpret_cast<const quint8*>(frame.constData() + 2);
    appendU16Le(frame, TypedTransportParser::crc16Ccitt(crcStart, frame.size() - 2));
    return frame;
}

QByteArray typedCanRawFrame(TypedRecordType type, quint16 seq, quint64 monoUs, quint32 canId, quint8 bus, const QByteArray& data) {
    QByteArray payload;
    appendU64Le(payload, monoUs);
    appendU32Le(payload, canId);
    const int dlc = std::min<int>(data.size(), 8);
    payload.append(char(dlc & 0x0F));
    payload.append(char(bus));
    for (int index = 0; index < 8; ++index) {
        payload.append(index < dlc ? data.at(index) : char(0));
    }
    appendU32Le(payload, seq);
    appendU32Le(payload, 0);
    return typedFrame(type, seq, payload);
}

QString writeModelFixture(const QString& rootPath) {
    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("can-monitor-model-pack.v1"));
    root.insert(QStringLiteral("model_key"), QStringLiteral("app_controller_replay_fixture"));
    root.insert(QStringLiteral("model_name"), QStringLiteral("Replay Flow Fixture"));
    root.insert(QStringLiteral("model_version"), QStringLiteral("2026-04-17"));
    root.insert(QStringLiteral("vendor"), QStringLiteral("Codex"));

    QJsonObject rule;
    rule.insert(QStringLiteral("id"), QStringLiteral("0x321"));
    rule.insert(QStringLiteral("name_en"), QStringLiteral("REPLAY_FLOW_FIXTURE"));
    rule.insert(QStringLiteral("expected_period_ms"), 100.0);
    rule.insert(QStringLiteral("ttl_warn_ms"), 500.0);
    rule.insert(QStringLiteral("ttl_err_ms"), 1000.0);
    rule.insert(QStringLiteral("period_err_warn_pct"), 20.0);
    rule.insert(QStringLiteral("period_err_err_pct"), 50.0);

    QJsonObject signal;
    signal.insert(QStringLiteral("name"), QStringLiteral("Motor Temperature"));
    signal.insert(QStringLiteral("byte_index_1based"), 1);
    signal.insert(QStringLiteral("bit_text"), QStringLiteral("8..1"));
    signal.insert(QStringLiteral("length_bits"), 8);
    signal.insert(QStringLiteral("start_bit_lsb"), 0);
    signal.insert(QStringLiteral("bit_positions_lsb"), QJsonArray{});
    signal.insert(QStringLiteral("scale"), 1.0);
    signal.insert(QStringLiteral("offset"), 0.0);
    signal.insert(QStringLiteral("signed"), false);
    signal.insert(QStringLiteral("range_text"), QStringLiteral("0 to 100"));
    signal.insert(QStringLiteral("operating_text"), QStringLiteral("unit: 1 C"));
    signal.insert(QStringLiteral("description"), QStringLiteral("fixture temperature"));
    signal.insert(QStringLiteral("reserved"), false);
    signal.insert(QStringLiteral("unit"), QStringLiteral("C"));
    signal.insert(QStringLiteral("alarm_mode"), QStringLiteral("range"));
    signal.insert(QStringLiteral("warn_max"), 50.0);
    signal.insert(QStringLiteral("err_max"), 60.0);
    signal.insert(QStringLiteral("alarm_severity"), QStringLiteral("ERR"));
    signal.insert(QStringLiteral("alarm_message"), QStringLiteral("Temperature too high"));

    QJsonObject message;
    message.insert(QStringLiteral("id"), QStringLiteral("0x321"));
    message.insert(QStringLiteral("name"), QStringLiteral("Replay Flow Frame"));
    message.insert(QStringLiteral("signals"), QJsonArray{signal});

    root.insert(QStringLiteral("rules"), QJsonArray{rule});
    root.insert(QStringLiteral("messages"), QJsonArray{message});

    const QString path = rootPath + QStringLiteral("/replay_flow_model.json");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return path;
}

QString writeReplayFixture(const QString& rootPath) {
    const QString path = rootPath + QStringLiteral("/replay_flow.bin");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    file.write(framePacket(100000, 0x321, 0, 1, QByteArray::fromHex("2800000000000000")));
    file.write(framePacket(260000, 0x321, 0, 2, QByteArray::fromHex("4600000000000000")));
    file.close();
    return path;
}

QString writeTypedReplaySession(const QString& rootPath) {
    const QString sessionDir = rootPath + QStringLiteral("/typed_flow.typed");
    QDir().mkpath(sessionDir);

    QFile meta(sessionDir + QStringLiteral("/session.meta.json"));
    if (!meta.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    QJsonObject metaObj;
    metaObj.insert(QStringLiteral("format"), QStringLiteral("typed-evidence-stream-v1"));
    metaObj.insert(QStringLiteral("created_local"), QStringLiteral("2026-04-10T14:27:49.032"));
    metaObj.insert(QStringLiteral("stream_file"), QStringLiteral("capture.stream"));
    meta.write(QJsonDocument(metaObj).toJson(QJsonDocument::Compact));
    meta.close();

    QFile stream(sessionDir + QStringLiteral("/capture.stream"));
    if (!stream.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    stream.write(typedCanRawFrame(TypedRecordType::CanRxRaw, 1, 100000, 0x321, 0, QByteArray::fromHex("2800000000000000")));
    stream.write(typedCanRawFrame(TypedRecordType::CanTxRaw, 2, 120000, 0x510, 1, QByteArray::fromHex("4000000000000000")));
    stream.write(typedCanRawFrame(TypedRecordType::CanRxRaw, 3, 260000, 0x321, 0, QByteArray::fromHex("4600000000000000")));
    stream.close();
    return sessionDir;
}

} // namespace

class AppControllerReplayFlowTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
    }

    void replayFlowGeneratesIssueMarkersAndSupportsIssueSeek() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QByteArray appDataPath = QDir::toNativeSeparators(tempDir.path()).toUtf8();
        qputenv("APPDATA", appDataPath);
        qputenv("LOCALAPPDATA", appDataPath);

        const QString modelPath = writeModelFixture(tempDir.path());
        const QString replayPath = writeReplayFixture(tempDir.path());
        QVERIFY(!modelPath.isEmpty());
        QVERIFY(!replayPath.isEmpty());

        AppController controller;
        controller.clearSavedSession();
        controller.clearFrames();
        controller.setRulesPath(modelPath);

        QTRY_VERIFY(controller.modelActive());
        QCOMPARE(controller.rulesCount(), 1);
        QCOMPARE(controller.signalDbMessageCount(), 1);

        controller.loadReplay(replayPath);
        QTRY_VERIFY(controller.replayLoaded());
        QTRY_COMPARE(controller.replayFrameCount(), 2);
        QVERIFY(controller.replayPath().endsWith(QStringLiteral("replay_flow.bin")));

        QVERIFY(controller.jumpReplayToFrameIndex(1, QStringLiteral("test replay jump")));
        QTRY_VERIFY(!controller.replayRebuilding());
        QTRY_COMPARE(controller.replayCurrentIndex(), 1);
        QTRY_COMPARE(controller.replayObservedIdCount(), 1);
        QTRY_VERIFY(controller.timingModel()->rowCount() > 0);
        const QVariantMap timingTop = controller.timingModel()->get(0);
        QCOMPARE(timingTop.value(QStringLiteral("idText")).toString(), QStringLiteral("0X321"));
        QVERIFY(!timingTop.value(QStringLiteral("reason")).toString().trimmed().isEmpty());

        controller.playReplay(1000.0);
        QTRY_VERIFY(!controller.replayPlaying());
        QTRY_VERIFY(controller.replayValueMarkerCount() > 0);
        QTRY_VERIFY(controller.replayAlarmMarkerCount() > 0);
        QTRY_VERIFY(!controller.replayIssueMarkers().isEmpty());
        QVERIFY(controller.replayAnalysisHeld());

        QVERIFY(controller.seekReplayIssue(QStringLiteral("value"), 1));
        QTRY_COMPARE(controller.selectedValueId(), QStringLiteral("0X321"));
        QVERIFY(controller.seekReplayId(QStringLiteral("0X321"), 1));
        QTRY_COMPARE(controller.valueFilterId(), QStringLiteral("0X321"));
    }

    void typedReplaySessionLoadsCanRxFramesOnly() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QByteArray appDataPath = QDir::toNativeSeparators(tempDir.path()).toUtf8();
        qputenv("APPDATA", appDataPath);
        qputenv("LOCALAPPDATA", appDataPath);

        const QString sessionDir = writeTypedReplaySession(tempDir.path());
        QVERIFY(!sessionDir.isEmpty());

        AppController controller;
        controller.clearSavedSession();
        controller.clearFrames();

        controller.loadReplay(sessionDir);
        QTRY_VERIFY(controller.replayLoaded());
        QTRY_COMPARE(controller.replayFrameCount(), 2);
        QVERIFY(controller.replayPath().endsWith(QStringLiteral("typed_flow.typed")));

        QVERIFY(controller.jumpReplayToFrameIndex(1, QStringLiteral("typed replay jump")));
        QTRY_VERIFY(!controller.replayRebuilding());
        QTRY_COMPARE(controller.replayCurrentIndex(), 1);
        QTRY_COMPARE(controller.replayObservedIdCount(), 1);
    }
};

QTEST_GUILESS_MAIN(AppControllerReplayFlowTest)

#include "test_app_controller_replay_flow.moc"
