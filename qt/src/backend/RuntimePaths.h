#pragma once

#include <QString>

namespace RuntimePaths {

QString appDataRoot();
QString configRoot();
QString logsRoot();
QString snapshotsRoot();

QString ensureAppDataRoot();
QString ensureConfigRoot();
QString ensureLogsRoot();
QString ensureSnapshotsRoot();

} // namespace RuntimePaths
