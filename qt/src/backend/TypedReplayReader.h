#pragma once

#include "TypedRecords.h"

#include <QMap>
#include <QString>
#include <QVector>

class TypedReplayReader {
public:
    struct Fault {
        quint64 offset = 0;
        QString code;
        QString message;
    };

    struct RecordEntry {
        quint64 offset = 0;
        quint64 monoUs = 0;
        TypedRecord record;
    };

    struct Summary {
        quint64 bytesRead = 0;
        quint64 recordCount = 0;
        QMap<quint8, quint64> typeCounts;
        bool hasRecords = false;
        quint64 firstMonoUs = 0;
        quint64 lastMonoUs = 0;
        quint16 firstSeq = 0;
        quint16 lastSeq = 0;
        quint64 bytesDropped = 0;
        quint64 crcFailures = 0;
        quint64 lengthFailures = 0;
        quint64 versionWarnings = 0;
        quint64 seqGaps = 0;
        quint64 trailingBytes = 0;
    };

    bool loadFile(const QString& path, QString* errorOut = nullptr);
    void reset();

    const QVector<RecordEntry>& records() const { return m_records; }
    const QVector<Fault>& faults() const { return m_faults; }
    const Summary& summary() const { return m_summary; }

private:
    static qsizetype findSof(const QByteArray& bytes, qsizetype from);
    static void setError(QString* errorOut, const QString& message);
    void addFault(quint64 offset, const QString& code, const QString& message);

    QVector<RecordEntry> m_records;
    QVector<Fault> m_faults;
    Summary m_summary;
};
