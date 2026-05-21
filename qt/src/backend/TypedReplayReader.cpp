#include "TypedReplayReader.h"

#include "TypedTransportParser.h"

#include <QFile>

namespace {

QString byteOffsetText(quint64 offset) {
    return QStringLiteral("offset %1").arg(offset);
}

} // namespace

void TypedReplayReader::reset() {
    m_records.clear();
    m_faults.clear();
    m_summary = {};
}

void TypedReplayReader::setError(QString* errorOut, const QString& message) {
    if (errorOut) *errorOut = message;
}

qsizetype TypedReplayReader::findSof(const QByteArray& bytes, qsizetype from) {
    for (qsizetype index = qMax<qsizetype>(0, from); index + 1 < bytes.size(); ++index) {
        const auto b0 = quint8(bytes.at(index));
        const auto b1 = quint8(bytes.at(index + 1));
        if (b0 == kTypedTransportSof0 && b1 == kTypedTransportSof1) return index;
    }
    return -1;
}

void TypedReplayReader::addFault(quint64 offset, const QString& code, const QString& message) {
    Fault fault;
    fault.offset = offset;
    fault.code = code;
    fault.message = message;
    m_faults.append(fault);
}

bool TypedReplayReader::loadFile(const QString& path, QString* errorOut) {
    reset();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorOut, QStringLiteral("Failed to open typed replay stream: %1").arg(file.errorString()));
        return false;
    }

    const QByteArray bytes = file.readAll();
    m_summary.bytesRead = quint64(bytes.size());

    qsizetype pos = 0;
    bool haveLastSeq = false;
    quint16 lastSeq = 0;

    while (pos < bytes.size()) {
        const qsizetype sof = findSof(bytes, pos);
        if (sof < 0) {
            const qsizetype dropped = bytes.size() - pos;
            if (dropped > 0) {
                m_summary.bytesDropped += quint64(dropped);
                addFault(quint64(pos), QStringLiteral("garbage_tail"),
                         QStringLiteral("Dropped %1 byte(s) after last valid typed frame.").arg(dropped));
            }
            break;
        }

        if (sof > pos) {
            const qsizetype dropped = sof - pos;
            m_summary.bytesDropped += quint64(dropped);
            addFault(quint64(pos), QStringLiteral("garbage"),
                     QStringLiteral("Dropped %1 byte(s) before typed SOF.").arg(dropped));
            pos = sof;
        }

        const qsizetype remaining = bytes.size() - pos;
        if (remaining < kTypedTransportFrameOverhead) {
            m_summary.trailingBytes = quint64(remaining);
            addFault(quint64(pos), QStringLiteral("incomplete_frame"),
                     QStringLiteral("Incomplete typed frame header at %1.").arg(byteOffsetText(quint64(pos))));
            break;
        }

        const auto* p = reinterpret_cast<const quint8*>(bytes.constData() + pos);
        const quint16 payloadLength = typedReadU16Le(p + 7);
        if (payloadLength > kTypedTransportMaxPayloadLength) {
            ++m_summary.lengthFailures;
            addFault(quint64(pos), QStringLiteral("length"),
                     QStringLiteral("Typed payload length %1 exceeds maximum %2 at %3.")
                         .arg(payloadLength)
                         .arg(kTypedTransportMaxPayloadLength)
                         .arg(byteOffsetText(quint64(pos))));
            ++pos;
            ++m_summary.bytesDropped;
            continue;
        }

        const qsizetype frameLength = kTypedTransportFrameOverhead + qsizetype(payloadLength);
        if (remaining < frameLength) {
            m_summary.trailingBytes = quint64(remaining);
            addFault(quint64(pos), QStringLiteral("incomplete_frame"),
                     QStringLiteral("Incomplete typed frame body at %1; expected %2 byte(s), have %3.")
                         .arg(byteOffsetText(quint64(pos)))
                         .arg(frameLength)
                         .arg(remaining));
            break;
        }

        const quint16 expectedCrc = typedReadU16Le(p + 9 + payloadLength);
        const quint16 actualCrc = TypedTransportParser::crc16Ccitt(p + 2, kTypedTransportHeaderSize + payloadLength);
        if (expectedCrc != actualCrc) {
            ++m_summary.crcFailures;
            addFault(quint64(pos), QStringLiteral("crc"),
                     QStringLiteral("Typed frame CRC mismatch at %1.").arg(byteOffsetText(quint64(pos))));
            ++pos;
            ++m_summary.bytesDropped;
            continue;
        }

        TypedRecord record;
        record.header.version = p[2];
        record.header.recordType = p[3];
        record.header.flags = p[4];
        record.header.seq = typedReadU16Le(p + 5);
        record.header.payloadLength = payloadLength;
        record.payload = bytes.mid(pos + 9, payloadLength);
        record.frameBytes = bytes.mid(pos, frameLength);

        if (record.header.version != kTypedTransportVersion) {
            ++m_summary.versionWarnings;
            addFault(quint64(pos), QStringLiteral("version"),
                     QStringLiteral("Typed frame version %1 differs from expected %2 at %3.")
                         .arg(record.header.version)
                         .arg(kTypedTransportVersion)
                         .arg(byteOffsetText(quint64(pos))));
        }

        if (!haveLastSeq) {
            haveLastSeq = true;
            m_summary.firstSeq = record.header.seq;
        } else {
            const quint16 expectedSeq = quint16(lastSeq + 1);
            if (record.header.seq != expectedSeq) {
                ++m_summary.seqGaps;
                addFault(quint64(pos), QStringLiteral("seq_gap"),
                         QStringLiteral("Typed sequence gap at %1: expected %2, got %3.")
                             .arg(byteOffsetText(quint64(pos)))
                             .arg(expectedSeq)
                             .arg(record.header.seq));
            }
        }
        lastSeq = record.header.seq;
        m_summary.lastSeq = record.header.seq;

        const quint64 monoUs = typedRecordMonoUs(record);
        if (!m_summary.hasRecords) {
            m_summary.hasRecords = true;
            m_summary.firstMonoUs = monoUs;
        }
        m_summary.lastMonoUs = monoUs;
        m_summary.typeCounts[record.header.recordType] += 1;

        RecordEntry entry;
        entry.offset = quint64(pos);
        entry.monoUs = monoUs;
        entry.record = record;
        m_records.append(entry);

        pos += frameLength;
    }

    m_summary.recordCount = quint64(m_records.size());
    if (m_records.isEmpty()) {
        setError(errorOut, QStringLiteral("No valid typed replay records found in %1.").arg(path));
        return false;
    }

    return true;
}
