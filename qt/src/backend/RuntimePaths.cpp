#include "RuntimePaths.h"

#include <QDir>
#include <QStandardPaths>

namespace {

QString normalizePath(QString path) {
    path = QDir::fromNativeSeparators(path);
    if (path.isEmpty()) {
        path = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    }
    return path;
}

QString ensurePath(const QString& path) {
    const QString normalized = normalizePath(path);
    QDir dir;
    dir.mkpath(normalized);
    return normalized;
}

} // namespace

namespace RuntimePaths {

QString appDataRoot() {
    return normalizePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

QString configRoot() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (path.isEmpty()) {
        path = QDir(appDataRoot()).filePath(QStringLiteral("config"));
    }
    return normalizePath(path);
}

QString logsRoot() {
    return QDir(appDataRoot()).filePath(QStringLiteral("logs"));
}

QString snapshotsRoot() {
    return QDir(appDataRoot()).filePath(QStringLiteral("snapshots"));
}

QString ensureAppDataRoot() {
    return ensurePath(appDataRoot());
}

QString ensureConfigRoot() {
    return ensurePath(configRoot());
}

QString ensureLogsRoot() {
    return ensurePath(logsRoot());
}

QString ensureSnapshotsRoot() {
    return ensurePath(snapshotsRoot());
}

} // namespace RuntimePaths
