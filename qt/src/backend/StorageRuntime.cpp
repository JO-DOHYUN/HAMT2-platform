#include "StorageRuntime.h"

#include "FilePersistence.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>

namespace {

void appendU16Le(QByteArray& out, quint16 value) {
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
}

void appendU64Le(QByteArray& out, quint64 value) {
    for (int byte = 0; byte < 8; ++byte) {
        out.append(char((value >> (byte * 8)) & 0xFF));
    }
}

} // namespace

StorageRuntime::Paths StorageRuntime::makePaths(const QString& sessionDir) {
    QDir dir(sessionDir);
    const QString root = dir.absolutePath();
    Paths paths;
    paths.sessionDir = root;
    paths.streamPart = dir.filePath(QStringLiteral("capture.stream.part"));
    paths.streamFinal = dir.filePath(QStringLiteral("capture.stream"));
    paths.indexPart = dir.filePath(QStringLiteral("capture.index.part"));
    paths.indexFinal = dir.filePath(QStringLiteral("capture.index"));
    paths.metaPart = dir.filePath(QStringLiteral("session.meta.json.part"));
    paths.metaFinal = dir.filePath(QStringLiteral("session.meta.json"));
    paths.eventsPart = dir.filePath(QStringLiteral("events.jsonl.part"));
    paths.eventsFinal = dir.filePath(QStringLiteral("events.jsonl"));
    return paths;
}

void StorageRuntime::setError(QString* errorOut, const QString& message) {
    if (errorOut) *errorOut = message;
}

bool StorageRuntime::startTypedSession(const QString& sessionDir, const QJsonObject& metadata, QString* errorOut) {
    discard();
    m_paths = makePaths(sessionDir);
    QDir dir(m_paths.sessionDir);
    if (!dir.mkpath(QStringLiteral("."))) {
        setError(errorOut, QStringLiteral("Failed to create typed session directory: %1").arg(m_paths.sessionDir));
        return false;
    }

    m_stream.setFileName(m_paths.streamPart);
    if (!m_stream.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(errorOut, m_stream.errorString());
        return false;
    }

    m_index.setFileName(m_paths.indexPart);
    if (!m_index.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(errorOut, m_index.errorString());
        closeFiles();
        FilePersistence::removeFileIfExists(m_paths.streamPart);
        return false;
    }

    m_events.setFileName(m_paths.eventsPart);
    if (!m_events.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setError(errorOut, m_events.errorString());
        closeFiles();
        FilePersistence::removeFileIfExists(m_paths.streamPart);
        FilePersistence::removeFileIfExists(m_paths.indexPart);
        return false;
    }

    QJsonObject meta = metadata;
    meta.insert(QStringLiteral("format"), QStringLiteral("typed-evidence-stream-v1"));
    meta.insert(QStringLiteral("transport_version"), int(kTypedTransportVersion));
    meta.insert(QStringLiteral("created_local"), QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
    meta.insert(QStringLiteral("stream_file"), QStringLiteral("capture.stream"));
    meta.insert(QStringLiteral("index_file"), QStringLiteral("capture.index"));
    meta.insert(QStringLiteral("events_file"), QStringLiteral("events.jsonl"));

    QString metaError;
    if (!FilePersistence::writeJsonAtomically(m_paths.metaPart, QJsonDocument(meta), &metaError)) {
        setError(errorOut, metaError);
        discard();
        return false;
    }

    m_active = true;
    m_recordCount = 0;
    m_bytesWritten = 0;
    return true;
}

bool StorageRuntime::appendTypedRecord(const TypedRecord& record, QString* errorOut) {
    if (!m_active || !m_stream.isOpen() || !m_index.isOpen()) {
        setError(errorOut, QStringLiteral("Typed storage session is not active."));
        return false;
    }
    if (record.frameBytes.size() < kTypedTransportFrameOverhead) {
        setError(errorOut, QStringLiteral("Typed record has no complete frame bytes."));
        return false;
    }

    const quint64 offset = quint64(m_stream.pos());
    const qint64 written = m_stream.write(record.frameBytes);
    if (written != record.frameBytes.size()) {
        setError(errorOut, m_stream.errorString().isEmpty() ? QStringLiteral("Failed to write typed stream bytes.") : m_stream.errorString());
        return false;
    }

    QByteArray indexEntry;
    indexEntry.reserve(24);
    appendU64Le(indexEntry, offset);
    appendU64Le(indexEntry, typedRecordMonoUs(record));
    indexEntry.append(char(record.header.recordType));
    indexEntry.append(char(record.header.flags));
    appendU16Le(indexEntry, record.header.seq);
    appendU16Le(indexEntry, record.header.payloadLength);
    appendU16Le(indexEntry, 0);

    const qint64 indexWritten = m_index.write(indexEntry);
    if (indexWritten != indexEntry.size()) {
        setError(errorOut, m_index.errorString().isEmpty() ? QStringLiteral("Failed to write typed index entry.") : m_index.errorString());
        return false;
    }

    ++m_recordCount;
    m_bytesWritten += quint64(written);
    return true;
}

bool StorageRuntime::appendEventJsonLine(const QJsonObject& event, QString* errorOut) {
    if (!m_active || !m_events.isOpen()) {
        setError(errorOut, QStringLiteral("Typed storage session is not active."));
        return false;
    }
    const QByteArray line = QJsonDocument(event).toJson(QJsonDocument::Compact) + '\n';
    const qint64 written = m_events.write(line);
    if (written != line.size()) {
        setError(errorOut, m_events.errorString().isEmpty() ? QStringLiteral("Failed to write typed event line.") : m_events.errorString());
        return false;
    }
    return true;
}

bool StorageRuntime::replacePartFile(const QString& partPath, const QString& finalPath, QString* errorOut) {
    if (!QFileInfo::exists(partPath)) {
        setError(errorOut, QStringLiteral("Missing typed session part file: %1").arg(partPath));
        return false;
    }
    if (QFileInfo::exists(finalPath) && !QFile::remove(finalPath)) {
        setError(errorOut, QStringLiteral("Failed to replace existing typed session file: %1").arg(finalPath));
        return false;
    }
    if (!QFile::rename(partPath, finalPath)) {
        setError(errorOut, QStringLiteral("Failed to finalize typed session file: %1").arg(finalPath));
        return false;
    }
    return true;
}

bool StorageRuntime::finalizeTypedSession(QString* errorOut) {
    if (!m_active) return true;

    m_stream.flush();
    m_index.flush();
    m_events.flush();
    closeFiles();

    if (!replacePartFile(m_paths.streamPart, m_paths.streamFinal, errorOut)) return false;
    if (!replacePartFile(m_paths.indexPart, m_paths.indexFinal, errorOut)) return false;
    if (!replacePartFile(m_paths.metaPart, m_paths.metaFinal, errorOut)) return false;
    if (!replacePartFile(m_paths.eventsPart, m_paths.eventsFinal, errorOut)) return false;
    m_active = false;
    return true;
}

void StorageRuntime::closeFiles() {
    if (m_stream.isOpen()) m_stream.close();
    if (m_index.isOpen()) m_index.close();
    if (m_events.isOpen()) m_events.close();
}

void StorageRuntime::discard() {
    closeFiles();
    if (!m_paths.streamPart.isEmpty()) FilePersistence::removeFileIfExists(m_paths.streamPart);
    if (!m_paths.indexPart.isEmpty()) FilePersistence::removeFileIfExists(m_paths.indexPart);
    if (!m_paths.metaPart.isEmpty()) FilePersistence::removeFileIfExists(m_paths.metaPart);
    if (!m_paths.eventsPart.isEmpty()) FilePersistence::removeFileIfExists(m_paths.eventsPart);
    m_active = false;
    m_recordCount = 0;
    m_bytesWritten = 0;
}
