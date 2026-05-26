#include "AppController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

QString writeModelFixture(const QString& rootPath) {
    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("can-monitor-model-pack.v1"));
    root.insert(QStringLiteral("model_key"), QStringLiteral("app_controller_log_fixture"));
    root.insert(QStringLiteral("model_name"), QStringLiteral("Log Flow Fixture"));
    root.insert(QStringLiteral("model_version"), QStringLiteral("2026-04-17"));
    root.insert(QStringLiteral("vendor"), QStringLiteral("Codex"));

    QJsonObject rule;
    rule.insert(QStringLiteral("id"), QStringLiteral("0x321"));
    rule.insert(QStringLiteral("name_en"), QStringLiteral("LOG_FLOW_FIXTURE"));
    rule.insert(QStringLiteral("expected_period_ms"), 100.0);

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

    QJsonObject message;
    message.insert(QStringLiteral("id"), QStringLiteral("0x321"));
    message.insert(QStringLiteral("name"), QStringLiteral("Log Flow Frame"));
    message.insert(QStringLiteral("signals"), QJsonArray{signal});

    root.insert(QStringLiteral("rules"), QJsonArray{rule});
    root.insert(QStringLiteral("messages"), QJsonArray{message});

    const QString path = rootPath + QStringLiteral("/log_flow_model.json");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return {};
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return path;
}

} // namespace

class AppControllerLogFlowTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
    }

    void controlEvidenceStatsStartAtZero() {
        AppController controller;
        const QString stats = controller.controlEvidenceStatsSummary();

        QVERIFY(stats.contains(QStringLiteral("요청 0")));
        QVERIFY(stats.contains(QStringLiteral("Qt write OK/Fail 0/0")));
        QVERIFY(stats.contains(QStringLiteral("ACK OK/Reject 0/0")));
        QVERIFY(stats.contains(QStringLiteral("TX audit 대기/매칭/미매칭 0/0/0")));
    }

    void boardEvidenceModelExposesReleaseGateRows() {
        AppController controller;
        auto* model = controller.boardEvidenceModel();
        QVERIFY(model != nullptr);
        QVERIFY2(model->count() >= 10, qPrintable(QStringLiteral("boardEvidence row count %1").arg(model->count())));

        const int serialRow = model->findIndex(QStringLiteral("key"), QStringLiteral("serial"));
        const int aliveRow = model->findIndex(QStringLiteral("key"), QStringLiteral("alive"));
        const int controlGateRow = model->findIndex(QStringLiteral("key"), QStringLiteral("control_gate"));
        const int txAuditRow = model->findIndex(QStringLiteral("key"), QStringLiteral("can_tx"));
        QVERIFY(serialRow >= 0);
        QVERIFY(aliveRow >= 0);
        QVERIFY(controlGateRow >= 0);
        QVERIFY(txAuditRow >= 0);

        QCOMPARE(model->get(serialRow).value(QStringLiteral("value")).toString(), QStringLiteral("CLOSED"));
        QCOMPARE(model->get(aliveRow).value(QStringLiteral("value")).toString(), QStringLiteral("NOT ALIVE"));
        QCOMPARE(model->get(controlGateRow).value(QStringLiteral("value")).toString(), QStringLiteral("BLOCKED"));
        QVERIFY(model->get(txAuditRow).value(QStringLiteral("note")).toString().contains(QStringLiteral("audit")));
    }

    void finalizePendingLogSaveCopiesArtifactsAndClearsPendingState() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QByteArray appDataPath = QDir::toNativeSeparators(tempDir.path()).toUtf8();
        qputenv("APPDATA", appDataPath);
        qputenv("LOCALAPPDATA", appDataPath);

        const QString modelPath = writeModelFixture(tempDir.path());
        QVERIFY(!modelPath.isEmpty());

        AppController controller;
        controller.clearSavedSession();
        controller.clearFrames();
        controller.setTransportMode(QStringLiteral("legacy20"));
        controller.setRulesPath(modelPath);
        QTRY_VERIFY_WITH_TIMEOUT(controller.modelActive(), 10000);

        controller.m_connected = true;
        controller.startLog();
        QTRY_VERIFY_WITH_TIMEOUT(controller.logRecordingActive(), 10000);
        QVERIFY(!controller.logPath().isEmpty());
        QVERIFY(QFileInfo::exists(controller.logPath()));
        QCOMPARE(controller.logPendingSave(), false);

        controller.stopLog();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.logRecordingActive(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(controller.logPendingSave(), 10000);
        QVERIFY(controller.logStatusSummary().contains(QStringLiteral("저장"), Qt::CaseInsensitive));

        const QString saveBasePath = tempDir.path() + QStringLiteral("/saved/capture_one");
        controller.finalizePendingLogSave(saveBasePath);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.logPendingSave(), 10000);
        QCOMPARE(controller.logSaving(), false);

        const QString finalBin = tempDir.path() + QStringLiteral("/saved/capture_one.bin");
        const QString finalMeta = tempDir.path() + QStringLiteral("/saved/capture_one.meta.json");
        const QString finalModel = tempDir.path() + QStringLiteral("/saved/capture_one.model.json");
        QCOMPARE(controller.logPath(), finalBin);
        QCOMPARE(controller.suggestedLogSavePath(), finalBin);
        QCOMPARE(controller.m_logLastSavedPath, finalBin);
        QVERIFY(QFileInfo::exists(finalBin));
        QVERIFY(QFileInfo::exists(finalMeta));
        QVERIFY(QFileInfo::exists(finalModel));
    }

    void discardPendingLogRemovesTempArtifactsAndClearsPendingState() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QByteArray appDataPath = QDir::toNativeSeparators(tempDir.path()).toUtf8();
        qputenv("APPDATA", appDataPath);
        qputenv("LOCALAPPDATA", appDataPath);

        const QString modelPath = writeModelFixture(tempDir.path());
        QVERIFY(!modelPath.isEmpty());

        AppController controller;
        controller.clearSavedSession();
        controller.clearFrames();
        controller.setTransportMode(QStringLiteral("legacy20"));
        controller.setRulesPath(modelPath);
        QTRY_VERIFY_WITH_TIMEOUT(controller.modelActive(), 10000);

        controller.m_connected = true;
        controller.startLog();
        QTRY_VERIFY_WITH_TIMEOUT(controller.logRecordingActive(), 10000);
        const QString tempBin = controller.m_logTempPath;
        const QString tempMeta = controller.m_logTempMetaPath;
        const QString tempModel = controller.m_logTempModelPath;
        QVERIFY(!tempBin.isEmpty());

        controller.stopLog();
        QTRY_VERIFY_WITH_TIMEOUT(controller.logPendingSave(), 10000);
        controller.discardPendingLog();

        QCOMPARE(controller.logPendingSave(), false);
        QCOMPARE(controller.logRecordingActive(), false);
        QCOMPARE(controller.logStopping(), false);
        QCOMPARE(controller.logSaving(), false);
        QVERIFY(!QFileInfo::exists(tempBin));
        QVERIFY(!QFileInfo::exists(tempMeta));
        QVERIFY(!QFileInfo::exists(tempModel));
        QVERIFY(controller.logPath().isEmpty());
    }
};

QTEST_GUILESS_MAIN(AppControllerLogFlowTest)

#include "test_app_controller_log_flow.moc"
