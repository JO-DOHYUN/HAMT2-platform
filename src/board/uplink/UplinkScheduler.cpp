#include "board/uplink/UplinkScheduler.h"

#include <Arduino.h>
#include <string.h>

namespace csm::board::uplink {

void UplinkScheduler::begin(SerialTxScheduler* serial_tx) {
  serial_tx_ = serial_tx;
  typed_seq_ = 0;
  counters_ = {};
  resetQueues();
}

bool UplinkScheduler::enqueueRecord(csm::RecordType type, const uint8_t* payload,
                                    uint16_t len, UplinkPriority priority,
                                    uint8_t flags) {
  if (len > csm::kMaxPayloadLen || (len > 0 && payload == nullptr)) {
    noteDrop(priority, true);
    return false;
  }

  PendingRecord record;
  record.type = type;
  record.len = len;
  record.flags = flags;
  record.priority = priority;
  if (len > 0) {
    memcpy(record.payload, payload, len);
  }

  RecordQueue& queue = queueFor(priority);
  if (!push(queue, record)) {
    noteDrop(priority);
    return false;
  }

  queued_payload_bytes_ += len;
  counters_.record_accept_total++;
  noteQueueHighWater(priority, queue.count);
  noteQueuedPayloadHighWater();
  return true;
}

SerialTxServiceResult UplinkScheduler::service(uint32_t byte_budget, uint32_t now_ms,
                                               uint32_t now_us) {
  SerialTxServiceResult result;
  if (serial_tx_ == nullptr || byte_budget == 0) {
    return result;
  }

  accumulate(result, serial_tx_->service(byte_budget, now_ms, now_us));
  if (result.actual_bytes >= byte_budget || result.writes_attempted > 0 ||
      serial_tx_->hasActiveFrame() || serial_tx_->backpressureActive()) {
    return result;
  }

  PendingRecord record;
  if (!popNext(record)) {
    return result;
  }

  if (!encodeAndStage(record)) {
    return result;
  }

  const uint32_t remaining = byte_budget - result.actual_bytes;
  if (remaining > 0) {
    accumulate(result, serial_tx_->service(remaining, now_ms, micros()));
  }
  return result;
}

bool UplinkScheduler::hasQueuedRecords() const {
  return queuedRecordCount() > 0;
}

bool UplinkScheduler::hasQueueSpace(UplinkPriority priority) const {
  const RecordQueue& queue = queueFor(priority);
  return queue.records != nullptr && queue.count < queue.capacity;
}

bool UplinkScheduler::pressureActive() const {
  if (serial_tx_ != nullptr &&
      (serial_tx_->backpressureActive() || serial_tx_->hasActiveFrame())) {
    return true;
  }
  return normal_queue_.count >= normal_queue_.capacity ||
         diagnostic_queue_.count >= diagnostic_queue_.capacity ||
         critical_queue_.count >= ((critical_queue_.capacity * 3U) / 4U);
}

uint32_t UplinkScheduler::queuedRecordCount() const {
  return static_cast<uint32_t>(critical_queue_.count) + normal_queue_.count +
         diagnostic_queue_.count;
}

void UplinkScheduler::resetQueues() {
  critical_queue_ =
      RecordQueue{critical_records_, BOARD_UPLINK_CRITICAL_QUEUE_RECORDS, 0, 0, 0};
  normal_queue_ =
      RecordQueue{normal_records_, BOARD_UPLINK_NORMAL_QUEUE_RECORDS, 0, 0, 0};
  diagnostic_queue_ =
      RecordQueue{diagnostic_records_, BOARD_UPLINK_DIAGNOSTIC_QUEUE_RECORDS, 0, 0, 0};
  queued_payload_bytes_ = 0;
}

UplinkScheduler::RecordQueue& UplinkScheduler::queueFor(UplinkPriority priority) {
  if (priority == UplinkPriority::Critical) {
    return critical_queue_;
  }
  if (priority == UplinkPriority::Diagnostic) {
    return diagnostic_queue_;
  }
  return normal_queue_;
}

const UplinkScheduler::RecordQueue& UplinkScheduler::queueFor(
    UplinkPriority priority) const {
  if (priority == UplinkPriority::Critical) {
    return critical_queue_;
  }
  if (priority == UplinkPriority::Diagnostic) {
    return diagnostic_queue_;
  }
  return normal_queue_;
}

bool UplinkScheduler::push(RecordQueue& queue, const PendingRecord& record) {
  if (queue.records == nullptr || queue.capacity == 0 || queue.count >= queue.capacity) {
    return false;
  }
  queue.records[queue.tail] = record;
  queue.tail = static_cast<uint8_t>((queue.tail + 1U) % queue.capacity);
  queue.count++;
  return true;
}

bool UplinkScheduler::pop(RecordQueue& queue, PendingRecord& record) {
  if (queue.records == nullptr || queue.count == 0) {
    return false;
  }
  record = queue.records[queue.head];
  queue.head = static_cast<uint8_t>((queue.head + 1U) % queue.capacity);
  queue.count--;
  if (queued_payload_bytes_ >= record.len) {
    queued_payload_bytes_ -= record.len;
  } else {
    queued_payload_bytes_ = 0;
  }
  return true;
}

bool UplinkScheduler::popNext(PendingRecord& record) {
  if (pop(critical_queue_, record)) {
    return true;
  }
  if (pop(normal_queue_, record)) {
    return true;
  }
  return pop(diagnostic_queue_, record);
}

void UplinkScheduler::noteDrop(UplinkPriority priority, bool encode_failure) {
  counters_.record_drop_total++;
  if (encode_failure) {
    counters_.record_encode_fail_total++;
  }
  switch (priority) {
    case UplinkPriority::Critical:
      counters_.critical_drop_total++;
      break;
    case UplinkPriority::Diagnostic:
      counters_.diagnostic_drop_total++;
      break;
    case UplinkPriority::Normal:
    default:
      counters_.normal_drop_total++;
      break;
  }
}

void UplinkScheduler::noteQueueHighWater(UplinkPriority priority, uint32_t count) {
  if (priority == UplinkPriority::Critical) {
    if (count > counters_.critical_queue_high_water) {
      counters_.critical_queue_high_water = count;
    }
    return;
  }
  if (priority == UplinkPriority::Diagnostic) {
    if (count > counters_.diagnostic_queue_high_water) {
      counters_.diagnostic_queue_high_water = count;
    }
    return;
  }
  if (count > counters_.normal_queue_high_water) {
    counters_.normal_queue_high_water = count;
  }
}

void UplinkScheduler::noteQueuedPayloadHighWater() {
  if (queued_payload_bytes_ > counters_.queued_payload_high_water_bytes) {
    counters_.queued_payload_high_water_bytes = queued_payload_bytes_;
  }
}

bool UplinkScheduler::encodeAndStage(const PendingRecord& record) {
  if (serial_tx_ == nullptr || serial_tx_->hasActiveFrame()) {
    noteDrop(record.priority);
    return false;
  }

  const uint32_t frame_len =
      static_cast<uint32_t>(csm::encoded_typed_frame_len(record.len));
  if (!serial_tx_->canEnqueue(frame_len, record.priority)) {
    serial_tx_->noteAdmissionFailure(frame_len, record.priority);
    noteDrop(record.priority);
    return false;
  }

  uint8_t frame[csm::encoded_typed_frame_len(csm::kMaxPayloadLen)];
  size_t written = 0;
  if (!csm::encode_typed_frame(frame, sizeof(frame), record.type, record.payload,
                               record.len, typed_seq_, record.flags, &written)) {
    noteDrop(record.priority, true);
    return false;
  }

  if (written != frame_len ||
      !serial_tx_->enqueueAtomic(frame, static_cast<uint32_t>(written), record.priority)) {
    noteDrop(record.priority);
    return false;
  }

  typed_seq_++;
  counters_.record_tx_total++;
  return true;
}

void UplinkScheduler::accumulate(SerialTxServiceResult& total,
                                 const SerialTxServiceResult& next) {
  total.requested_bytes += next.requested_bytes;
  total.actual_bytes += next.actual_bytes;
  total.writes_attempted += next.writes_attempted;
  total.writes_completed += next.writes_completed;
  total.partial_writes += next.partial_writes;
  total.backpressure_event = total.backpressure_event || next.backpressure_event;
  if (next.backpressure_duration_ms > total.backpressure_duration_ms) {
    total.backpressure_duration_ms = next.backpressure_duration_ms;
  }
}

}  // namespace csm::board::uplink
