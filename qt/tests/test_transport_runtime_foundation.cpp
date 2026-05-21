#include "transport/HostTxQueue.h"

#include <QtTest/QtTest>

class TransportRuntimeFoundationTest : public QObject {
    Q_OBJECT

private slots:
    void hostTxQueuePreservesFifoAndCounters() {
        CanMonitorTransport::HostTxQueue queue(4, 128);

        QString error;
        QVERIFY(queue.enqueue(QByteArray::fromHex("A55A010A00010000AA00"), QStringLiteral("first"), &error));
        QVERIFY(error.isEmpty());
        QVERIFY(queue.enqueue(QByteArray::fromHex("A55A010A00010000BB00"), QStringLiteral("second"), &error));

        QCOMPARE(queue.queuedFrames(), qsizetype(2));
        QCOMPARE(queue.enqueuedFrames(), quint64(2));
        QCOMPARE(queue.writtenFrames(), quint64(0));
        QCOMPARE(queue.droppedFrames(), quint64(0));

        const auto first = queue.dequeue();
        QCOMPARE(first.sequence, quint64(1));
        QCOMPARE(first.summary, QStringLiteral("first"));
        queue.markWritten();

        const auto second = queue.dequeue();
        QCOMPARE(second.sequence, quint64(2));
        QCOMPARE(second.summary, QStringLiteral("second"));
        queue.markWritten();

        QVERIFY(!queue.hasPending());
        QCOMPARE(queue.writtenFrames(), quint64(2));
    }

    void hostTxQueueRejectsOverflowWithoutDroppingQueuedItems() {
        CanMonitorTransport::HostTxQueue queue(1, 16);

        QString error;
        QVERIFY(queue.enqueue(QByteArray(12, char(0x11)), QStringLiteral("first"), &error));
        QVERIFY(!queue.enqueue(QByteArray(4, char(0x22)), QStringLiteral("second"), &error));
        QVERIFY(error.contains(QStringLiteral("full")));
        QCOMPARE(queue.queuedFrames(), qsizetype(1));
        QCOMPARE(queue.droppedFrames(), quint64(1));

        const auto first = queue.dequeue();
        QCOMPARE(first.summary, QStringLiteral("first"));
    }

    void hostTxQueueRejectsByteLimit() {
        CanMonitorTransport::HostTxQueue queue(4, 8);

        QString error;
        QVERIFY(!queue.enqueue(QByteArray(9, char(0x33)), QStringLiteral("large"), &error));
        QVERIFY(error.contains(QStringLiteral("byte")));
        QCOMPARE(queue.queuedFrames(), qsizetype(0));
        QCOMPARE(queue.droppedFrames(), quint64(1));
    }
};

QTEST_APPLESS_MAIN(TransportRuntimeFoundationTest)

#include "test_transport_runtime_foundation.moc"
