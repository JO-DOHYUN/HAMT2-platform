#include "AppController.h"
#include "CanTypes.h"
#include "GraphViewportItem.h"
#include "TypedRecords.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QUrl>
#include <QtTest/QtTest>

namespace {

QStringList g_qmlErrors;

void qmlSmokeMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    Q_UNUSED(context);
    if (type != QtWarningMsg && type != QtCriticalMsg && type != QtFatalMsg) return;

    const QString lower = message.toLower();
    const bool looksLikeQmlFailure =
        message.contains(QStringLiteral(".qml")) ||
        lower.contains(QStringLiteral("referenceerror")) ||
        lower.contains(QStringLiteral("typeerror")) ||
        lower.contains(QStringLiteral("cannot assign")) ||
        lower.contains(QStringLiteral("failed to load"));
    if (looksLikeQmlFailure) g_qmlErrors.push_back(message);
}

void registerRuntimeTypes() {
    qRegisterMetaType<FrameRecord>("FrameRecord");
    qRegisterMetaType<FrameRecordList>("FrameRecordList");
    qRegisterMetaType<StatsRecord>("StatsRecord");
    qRegisterMetaType<TypedRecord>("TypedRecord");
    qRegisterMetaType<TypedRecordList>("TypedRecordList");
    qmlRegisterType<GraphViewportItem>("CanMonitor", 1, 0, "GraphViewport");
}

} // namespace

class QmlShellSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void loadsEveryTabAtSupportedScales() {
        registerRuntimeTypes();
        qInstallMessageHandler(qmlSmokeMessageHandler);
        g_qmlErrors.clear();

        AppController controller;
        QQmlApplicationEngine engine;
        engine.addImportPath(QStringLiteral(CAN_MONITOR_QML_DIR));
        engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

        bool creationFailed = false;
        connect(&engine, &QQmlApplicationEngine::objectCreationFailed, this, [&creationFailed]() {
            creationFailed = true;
        });

        engine.load(QUrl::fromLocalFile(QStringLiteral(CAN_MONITOR_QML_DIR "/Main.qml")));
        QTRY_VERIFY_WITH_TIMEOUT(creationFailed || !engine.rootObjects().isEmpty(), 5000);
        QVERIFY2(!creationFailed, "QML object creation failed");
        QVERIFY2(!engine.rootObjects().isEmpty(), "QML root object was not created");

        QObject* root = engine.rootObjects().first();
        QVERIFY(root);
        QVERIFY2(root->property("visible").toBool(), "QML root window is not visible");

        const QStringList keys = {
            QStringLiteral("overview"),
            QStringLiteral("live"),
            QStringLiteral("replay"),
            QStringLiteral("timing"),
            QStringLiteral("value"),
            QStringLiteral("graph"),
            QStringLiteral("graph_overview"),
            QStringLiteral("alarm"),
            QStringLiteral("settings"),
            QStringLiteral("control"),
        };

        for (int scaleIndex = 0; scaleIndex < 3; ++scaleIndex) {
            QVERIFY(root->setProperty("uiScaleIndex", scaleIndex));
            QTest::qWait(40);
            for (const QString& key : keys) {
                const QVariant keyArg(key);
                QVERIFY2(QMetaObject::invokeMethod(root, "openPanelByKey", Q_ARG(QVariant, keyArg)),
                         qPrintable(QStringLiteral("openPanelByKey failed for %1").arg(key)));
                QTest::qWait(80);
                QVERIFY2(g_qmlErrors.isEmpty(), qPrintable(g_qmlErrors.join(QStringLiteral("\n"))));
            }
        }

        QVERIFY(root->setProperty("workspaceMode", true));
        QTest::qWait(80);
        for (const QString& key : {QStringLiteral("live"), QStringLiteral("graph_overview"), QStringLiteral("control")}) {
            const QVariant keyArg(key);
            QVERIFY(QMetaObject::invokeMethod(root, "openPanelByKey", Q_ARG(QVariant, keyArg)));
            QTest::qWait(80);
            QVERIFY2(g_qmlErrors.isEmpty(), qPrintable(g_qmlErrors.join(QStringLiteral("\n"))));
        }
    }
};

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows"));
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Fusion"));
    QGuiApplication app(argc, argv);
    QmlShellSmokeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qml_shell_smoke.moc"
