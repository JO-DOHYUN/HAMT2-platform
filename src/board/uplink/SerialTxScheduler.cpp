#include "board/uplink/SerialTxScheduler.h"

#include <Arduino.h>

#if defined(SERIAL_CDC)
#include "USB/PluggableUSBSerial.h"
#endif

#ifndef BOARD_SERIAL_TX_CHUNK_BYTES
#define BOARD_SERIAL_TX_CHUNK_BYTES 512
#endif

namespace csm::board::uplink {

void SerialTxScheduler::begin(uint8_t* ring, uint32_t ring_size,
                              const SerialTxSchedulerConfig& config) {
  ring_ = ring;
  ring_size_ = ring_size;
  mask_ = ring_size_ > 0 ? ring_size_ - 1 : 0;
  head_ = 0;
  tail_ = 0;
  blocked_since_ms_ = 0;
  normal_throttle_active_ = false;
  config_ = config;
  counters_ = {};
}

bool SerialTxScheduler::ready() const {
  return ring_ != nullptr && ring_size_ > 1 && ((ring_size_ & (ring_size_ - 1)) == 0);
}

uint32_t SerialTxScheduler::queuedBytes() const {
  if (!ready()) {
    return 0;
  }
  return (head_ - tail_) & mask_;
}

uint32_t SerialTxScheduler::capacityBytes() const {
  if (!ready()) {
    return 0;
  }
  return ring_size_ - 1;
}

uint32_t SerialTxScheduler::freeBytes() const {
  return capacityBytes() - queuedBytes();
}

uint32_t SerialTxScheduler::normalLimitBytes() const {
  const uint32_t capacity = capacityBytes();
  if (config_.critical_reserve_bytes >= capacity) {
    return capacity;
  }
  return capacity - config_.critical_reserve_bytes;
}

bool SerialTxScheduler::normalThrottleActive() const {
  return normal_throttle_active_;
}

bool SerialTxScheduler::backpressureActive() const {
  return blocked_since_ms_ != 0;
}

void SerialTxScheduler::updateNormalThrottle() {
  const uint32_t queued = queuedBytes();
  const uint32_t high = config_.normal_high_water_bytes;
  const uint32_t low = config_.normal_low_water_bytes;
  if (high == 0) {
    normal_throttle_active_ = false;
    return;
  }
  if (!normal_throttle_active_ && queued >= high) {
    normal_throttle_active_ = true;
    counters_.normal_throttle_total++;
  } else if (normal_throttle_active_ && queued <= low) {
    normal_throttle_active_ = false;
  }
}

SerialTxAdmissionResult SerialTxScheduler::admissionResult(uint32_t len,
                                                           UplinkPriority priority) const {
  if (!ready() || len > freeBytes()) {
    return ready() ? SerialTxAdmissionResult::NoSpace : SerialTxAdmissionResult::NotReady;
  }
  if (priority == UplinkPriority::Critical) {
    return SerialTxAdmissionResult::Accept;
  }
  if (backpressureActive()) {
    return SerialTxAdmissionResult::BackpressureSuppressed;
  }
  if (normal_throttle_active_) {
    return SerialTxAdmissionResult::NormalThrottle;
  }
  if (config_.normal_high_water_bytes > 0 &&
      queuedBytes() + len >= config_.normal_high_water_bytes) {
    return SerialTxAdmissionResult::NormalThrottle;
  }
  if (queuedBytes() + len > normalLimitBytes()) {
    return SerialTxAdmissionResult::ReserveProtected;
  }
  return SerialTxAdmissionResult::Accept;
}

bool SerialTxScheduler::canEnqueue(uint32_t len, UplinkPriority priority) const {
  return admissionResult(len, priority) == SerialTxAdmissionResult::Accept;
}

void SerialTxScheduler::noteAdmissionFailure(uint32_t len, UplinkPriority priority) {
  switch (admissionResult(len, priority)) {
    case SerialTxAdmissionResult::NotReady:
    case SerialTxAdmissionResult::NoSpace:
      counters_.enqueue_fail_total++;
      break;
    case SerialTxAdmissionResult::ReserveProtected:
      counters_.reserve_reject_total++;
      break;
    case SerialTxAdmissionResult::BackpressureSuppressed:
    case SerialTxAdmissionResult::NormalThrottle:
    case SerialTxAdmissionResult::Accept:
    default:
      break;
  }
}

bool SerialTxScheduler::enqueueAtomic(const uint8_t* data, uint32_t len,
                                      UplinkPriority priority) {
  if (len == 0) {
    return true;
  }
  if (data == nullptr) {
    counters_.enqueue_fail_total++;
    return false;
  }
  if (!canEnqueue(len, priority)) {
    noteAdmissionFailure(len, priority);
    return false;
  }

  for (uint32_t i = 0; i < len; ++i) {
    ring_[head_] = data[i];
    head_ = (head_ + 1) & mask_;
  }
  const uint32_t queued = queuedBytes();
  if (queued > counters_.high_water_bytes) {
    counters_.high_water_bytes = queued;
  }
  updateNormalThrottle();
  return true;
}

void SerialTxScheduler::rewindTail(uint32_t count) {
  while (count-- > 0) {
    tail_ = (tail_ - 1) & mask_;
  }
}

SerialTxServiceResult SerialTxScheduler::service(uint32_t byte_budget, uint32_t now_ms,
                                                 uint32_t now_us) {
  SerialTxServiceResult result;
  if (!ready()) {
    return result;
  }

#if defined(SERIAL_CDC)
  const uint32_t start_us = now_us;
  const uint32_t configured_bytes = config_.max_bytes_per_pump == 0
                                        ? byte_budget
                                        : config_.max_bytes_per_pump;
  uint32_t remaining_budget = byte_budget < configured_bytes ? byte_budget : configured_bytes;
  const uint32_t max_writes = config_.max_writes_per_pump == 0
                                  ? 0xFFFFFFFFu
                                  : config_.max_writes_per_pump;
  uint32_t writes = 0;
  uint8_t chunk[BOARD_SERIAL_TX_CHUNK_BYTES];
  while (remaining_budget > 0 && writes < max_writes && tail_ != head_) {
    if (config_.drain_time_budget_us > 0 &&
        static_cast<uint32_t>(micros() - start_us) >= config_.drain_time_budget_us) {
      break;
    }

    uint32_t n = 0;
    while (n < sizeof(chunk) && n < remaining_budget && tail_ != head_) {
      chunk[n++] = ring_[tail_];
      tail_ = (tail_ + 1) & mask_;
    }
    if (n == 0) {
      break;
    }

    uint32_t actual = 0;
    _SerialUSB.send_nb(chunk, n, &actual, true);
    writes++;
    counters_.write_attempt_total++;
    result.writes_attempted++;
    counters_.last_requested_bytes = n;
    counters_.last_actual_bytes = actual;
    counters_.requested_bytes_total += n;
    counters_.actual_bytes_total += actual;
    result.requested_bytes += n;
    result.actual_bytes += actual;

    if (actual == 0) {
      rewindTail(n);
      counters_.actual_zero_total++;
      if (blocked_since_ms_ == 0) {
        blocked_since_ms_ = now_ms;
        counters_.backpressure_total++;
      } else {
        const uint32_t duration = now_ms - blocked_since_ms_;
        if (duration > counters_.backpressure_max_duration_ms) {
          counters_.backpressure_max_duration_ms = duration;
        }
        result.backpressure_duration_ms = duration;
      }
      break;
    }

    if (actual == n) {
      counters_.write_complete_total++;
      result.writes_completed++;
    }

    if (blocked_since_ms_ != 0) {
      const uint32_t duration = now_ms - blocked_since_ms_;
      if (duration > counters_.backpressure_max_duration_ms) {
        counters_.backpressure_max_duration_ms = duration;
      }
      blocked_since_ms_ = 0;
      result.backpressure_event = true;
      result.backpressure_duration_ms = duration;
    }

    if (actual < n) {
      rewindTail(n - actual);
      if (actual > 0) {
        updateNormalThrottle();
      }
      counters_.partial_write_total++;
      result.partial_writes++;
      break;
    }
    remaining_budget -= actual;
    updateNormalThrottle();
  }
#else
  (void)byte_budget;
  (void)now_ms;
  (void)now_us;
#endif
  return result;
}

uint32_t SerialTxScheduler::clearDisconnectedStale() {
  const uint32_t dropped_bytes = queuedBytes();
  if (dropped_bytes > 0) {
    counters_.ring_clear_total++;
    counters_.ring_cleared_bytes_total += dropped_bytes;
  }
  head_ = 0;
  tail_ = 0;
  blocked_since_ms_ = 0;
  normal_throttle_active_ = false;
  return dropped_bytes;
}

}  // namespace csm::board::uplink
