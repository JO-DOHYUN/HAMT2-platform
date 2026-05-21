#include "backend/AppController.h"
#include "backend/AppLogging.h"
#include "backend/BuildMetadata.h"
#include "backend/CanTypes.h"
#include "backend/GraphViewportItem.h"
#include "backend/TypedRecords.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

int main(int argc, char* argv[]) {
    qRegisterMetaType<FrameRecord>("FrameRecord");
    qRegisterMetaType<FrameRecordList>("FrameRecordList");
    qRegisterMetaType<StatsRecord>("StatsRecord");
    qRegisterMetaType<TypedRecord>("TypedRecord");
    qRegisterMetaType<TypedRecordList>("TypedRecordList");

#ifdef Q_OS_WIN
    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
#endif

    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Fusion"));
    QGuiApplication app(argc, argv);
    const BuildMetadata::Info buildInfo = BuildMetadata::current();
    app.setOrganizationName(buildInfo.organizationName);
    app.setOrganizationDomain(buildInfo.organizationDomain);
    app.setApplicationName(buildInfo.applicationName);

    AppLogging::initialize(buildInfo);
    qCInfo(logUi).noquote() << "Startup:" << BuildMetadata::banner(buildInfo);

    AppController controller;

    qmlRegisterType<GraphViewportItem>("CanMonitor", 1, 0, "GraphViewport");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            qCCritical(logUi) << "QML object creation failed";
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.loadFromModule("CanMonitor", "Main");

    const int exitCode = app.exec();
    qCInfo(logUi) << "Shutdown with exit code" << exitCode;
    AppLogging::shutdown();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return exitCode;
}
