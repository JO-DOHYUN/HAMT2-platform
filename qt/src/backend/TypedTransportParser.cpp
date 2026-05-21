#include "TypedTransportParser.h"

#include <algorithm>

void TypedTransportParser::reset() {
    m_buffer.clear();
    m_counters = {};
    m_haveLastSeq = false;
    m_lastSeq = 0;
}

void TypedTransportParser::append(const QByteArray& chunk) {
    m_buffer.append(chunk);
}

quint16 TypedTransportParser::crc16Ccitt(const quint8* data, qsizetype len) {
    quint16 crc = 0xFFFF;
    for (qsizetype index = 0; index < len; ++index) {
        crc ^= quint16(data[index]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? quint16((crc << 1) ^ 0x1021) : quint16(crc << 1);
        }
    }
    return crc;
}

int TypedTransportParser::findSof(const QByteArray& buffer) {
    const int size = int(buffer.size());
    for (int index = 0; index + 1 < size; ++index) {
        const auto b0 = quint8(buffer.at(index));
        const auto b1 = quint8(buffer.at(index + 1));
        if (b0 == kTypedTransportSof0 && b1 == kTypedTransportSof1) return index;
    }
    return -1;
}

bool TypedTransportParser::noteSequence(quint16 seq) {
    if (!m_haveLastSeq) {
        m_haveLastSeq = true;
        m_lastSeq = seq;
        return true;
    }
    const quint16 expected = quint16(m_lastSeq + 1);
    const bool contiguous = seq == expected;
    if (!contiguous) ++m_counters.seqGaps;
    m_lastSeq = seq;
    return contiguous;
}

std::optional<TypedRecord> TypedTransportParser::takeOne() {
    while (true) {
        if (m_buffer.size() < kTypedTransportFrameOverhead) return std::nullopt;

        const int sof = findSof(m_buffer);
        if (sof < 0) {
            const qsizetype drop = std::max<qsizetype>(0, m_buffer.size() - 1);
            if (drop > 0) {
                m_buffer.remove(0, drop);
                m_counters.bytesDropped += quint64(drop);
            }
            return std::nullopt;
        }
        if (sof > 0) {
            m_buffer.remove(0, sof);
            m_counters.bytesDropped += quint64(sof);
        }
        if (m_buffer.size() < kTypedTransportFrameOverhead) return std::nullopt;

        const auto* p = reinterpret_cast<const quint8*>(m_buffer.constData());
        const quint16 payloadLength = typedReadU16Le(p + 7);
        if (payloadLength > kTypedTransportMaxPayloadLength) {
            ++m_counters.lengthFailures;
            m_buffer.remove(0, 1);
            ++m_counters.bytesDropped;
            continue;
        }

        const qsizetype frameLength = kTypedTransportFrameOverhead + qsizetype(payloadLength);
        if (m_buffer.size() < frameLength) return std::nullopt;

        const quint16 expectedCrc = typedReadU16Le(p + 9 + payloadLength);
        const quint16 actualCrc = crc16Ccitt(p + 2, kTypedTransportHeaderSize + payloadLength);
        if (expectedCrc != actualCrc) {
            ++m_counters.crcFailures;
            m_buffer.remove(0, 1);
            ++m_counters.bytesDropped;
            continue;
        }

        TypedRecord record;
        record.header.version = p[2];
        record.header.recordType = p[3];
        record.header.flags = p[4];
        record.header.seq = typedReadU16Le(p + 5);
        record.header.payloadLength = payloadLength;
        record.payload = m_buffer.mid(9, payloadLength);
        record.frameBytes = m_buffer.left(frameLength);

        if (record.header.version != kTypedTransportVersion) {
            ++m_counters.versionWarnings;
        }
        noteSequence(record.header.seq);
        ++m_counters.frames;

        m_buffer.remove(0, frameLength);
        return record;
    }
}
