#include "ControlCommandEncoder.h"
#include "TypedRecords.h"

#include <QtTest/QtTest>

using CanMonitorControl::ControlCommandEncoder;

class ControlCommandEncoderTest : public QObject {
    Q_OBJECT
private slots:
    void encodesAdcuManualAndVcuFrames() {
        const auto frames = ControlCommandEncoder::makeControlBurst(10000, 10000, -90.0, 1, 2, 7, 1);
        QCOMPARE(frames.size(), 5);

        const auto adcu = frames.at(0);
        QCOMPARE(adcu.canId, quint32(0x503));
        QCOMPARE(adcu.bus, quint8(1));
        QCOMPARE(adcu.data[0], quint8(0x80));
        QCOMPARE(adcu.data[1], quint8(0x00));
        QCOMPARE(adcu.data[2], quint8(0x00));
        QCOMPARE(adcu.data[3], quint8(0x00));
        QCOMPARE(adcu.data[4], quint8(0x00));
        QCOMPARE(adcu.data[5], quint8(0x40));
        QCOMPARE(adcu.data[7], quint8(0x70));

        const auto vcu = frames.at(1);
        QCOMPARE(vcu.canId, quint32(0x510));
        QCOMPARE(vcu.data[0], quint8(0x40));
        QCOMPARE(vcu.data[1], quint8(0x10));
        QCOMPARE(vcu.data[2], quint8(0x27));
        QCOMPARE(vcu.data[3], quint8(0x7C));
        QCOMPARE(vcu.data[4], quint8(0xFC));

        const auto neutral = ControlCommandEncoder::makeVcuAutoFrame(0x510, 0, 0.0, 1, 1);
        QCOMPARE(neutral.data[0], quint8(0x40));
        for (int index = 1; index < 8; ++index) QCOMPARE(neutral.data[index], quint8(0x00));
    }

    void encodesSignedVcuCommandFromVehicleEvidence() {
        const auto frames = ControlCommandEncoder::makeControlBurst(-4500, 4500, 2.0, 1, 2, 0, 1);
        QCOMPARE(frames.size(), 5);

        const auto vcu = frames.at(1);
        QCOMPARE(vcu.canId, quint32(0x510));
        QCOMPARE(vcu.data[0], quint8(0x40));
        QCOMPARE(vcu.data[1], quint8(0x6C));
        QCOMPARE(vcu.data[2], quint8(0xEE));
        QCOMPARE(vcu.data[3], quint8(0x14));
        QCOMPARE(vcu.data[4], quint8(0x00));
    }

    void buildsHostCanTxRequestFrame() {
        const auto frame = ControlCommandEncoder::makeVcuAutoFrame(0x513, 1200, 90.0, 1, 1);
        const QByteArray request = ControlCommandEncoder::buildHostCanTxRequest(0x1234, frame);
        QVERIFY(request.size() > kTypedTransportFrameOverhead);
        QCOMPARE(quint8(request.at(0)), kTypedTransportSof0);
        QCOMPARE(quint8(request.at(1)), kTypedTransportSof1);
        QCOMPARE(quint8(request.at(2)), kTypedTransportVersion);
        QCOMPARE(quint8(request.at(3)), CanMonitorControl::kHostCommandRecordTypeCanTxRequest);
        QCOMPARE(typedReadU16Le(reinterpret_cast<const quint8*>(request.constData() + 5)), quint16(0x1234));
        QCOMPARE(typedReadU16Le(reinterpret_cast<const quint8*>(request.constData() + 7)), quint16(19));

        const auto* payload = reinterpret_cast<const quint8*>(request.constData() + 9);
        QCOMPARE(typedReadU32Le(payload), quint32(0x1234));
        QCOMPARE(payload[4], quint8(1));
        QCOMPARE(typedReadU32Le(payload + 6), quint32(0x513));
        QCOMPARE(payload[10], quint8(8));
        QCOMPARE(payload[11], quint8(0x40));
        QCOMPARE(payload[12], quint8(0xB0));
        QCOMPARE(payload[13], quint8(0x04));
    }

    void buildsHostHeartbeatAndControlSessionFrames() {
        const QByteArray heartbeat = ControlCommandEncoder::buildHostHeartbeat(0x2001, 0x11223344);
        QCOMPARE(quint8(heartbeat.at(3)), CanMonitorControl::kHostCommandRecordTypeHeartbeat);
        QCOMPARE(typedReadU16Le(reinterpret_cast<const quint8*>(heartbeat.constData() + 7)), quint16(12));
        const auto* heartbeatPayload = reinterpret_cast<const quint8*>(heartbeat.constData() + 9);
        QCOMPARE(typedReadU32Le(heartbeatPayload), quint32(0x2001));
        QCOMPARE(typedReadU32Le(heartbeatPayload + 4), quint32(0x11223344));

        const QByteArray session = ControlCommandEncoder::buildHostControlSession(
            0x2002,
            CanMonitorControl::kControlSessionArm,
            CanMonitorControl::kControlSessionAnyBus,
            2000);
        QCOMPARE(quint8(session.at(3)), CanMonitorControl::kHostCommandRecordTypeControlSession);
        QCOMPARE(typedReadU16Le(reinterpret_cast<const quint8*>(session.constData() + 7)), quint16(24));
        const auto* sessionPayload = reinterpret_cast<const quint8*>(session.constData() + 9);
        QCOMPARE(typedReadU32Le(sessionPayload), quint32(0x2002));
        QCOMPARE(sessionPayload[4], CanMonitorControl::kControlSessionArm);
        QCOMPARE(sessionPayload[5], CanMonitorControl::kControlSessionAnyBus);
        QCOMPARE(typedReadU16Le(sessionPayload + 8), quint16(2000));
    }
};

QTEST_MAIN(ControlCommandEncoderTest)
#include "test_control_command_encoder.moc"
