#include "board/uplink/SerialTxScheduler.h"

#include <Arduino.h>

#if defined(SERIAL_CDC)
#include "USB/PluggableUSBSerial.h"
#endif

#ifndef BOARD_SERIAL_TX_CHUNK_BYTES
#define BOARD_SERIAL_TX_CHUNK_BYTES 128
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
  last_backpressure_event_ms_ = 0;
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

bool SerialTxScheduler::canEnqueue(uint32_t len, UplinkPriority priority) const {
  if (!ready() || len > freeBytes()) {
    return false;
  }
  if (priority == UplinkPriority::Critical) {
    return true;
  }
  return queuedBytes() + len <= normalLimitBytes();
}

void SerialTxScheduler::noteAdmissionFailure(uint32_t len, UplinkPriority priority) {
  counters_.enqueue_fail_total++;
  if (ready() && len <= freeBytes() && priority != UplinkPriority::Critical) {
    counters_.reserve_reject_total++;
  }
}

bool SerialTxScheduler::enqueueAtomic(const uint8_t* data, uint32_t len,
                                      UplinkPriority priority) {
  if (len == 0) {
    return true;
  }
  if (data == nullptr || !canEnqueue(len, priority)) {
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
  uint8_t chunk[BOARD_SERIAL_TX_CHUNK_BYTES];
  while (byte_budget > 0 && tail_ != head_) {
    if (config_.drain_time_budget_us > 0 &&
        static_cast<uint32_t>(micros() - start_us) >= config_.drain_time_budget_us) {
      break;
    }

    uint32_t n = 0;
    while (n < sizeof(chunk) && n < byte_budget && tail_ != head_) {
      chunk[n++] = ring_[tail_];
      tail_ = (tail_ + 1) & mask_;
    }
    if (n == 0) {
      break;
    }

    uint32_t actual = 0;
    _SerialUSB.send_nb(chunk, n, &actual, true);
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
        result.backpressure_event = true;
        last_backpressure_event_ms_ = now_ms;
      } else {
        const uint32_t duration = now_ms - blocked_since_ms_;
        if (duration > counters_.backpressure_max_duration_ms) {
          counters_.backpressure_max_duration_ms = duration;
        }
        result.backpressure_duration_ms = duration;
        if (config_.backpressure_event_period_ms > 0 &&
            now_ms - last_backpressure_event_ms_ >= config_.backpressure_event_period_ms) {
          result.backpressure_event = true;
          last_backpressure_event_ms_ = now_ms;
        }
      }
      break;
    }

    if (blocked_since_ms_ != 0) {
      const uint32_t duration = now_ms - blocked_since_ms_;
      if (duration > counters_.backpressure_max_duration_ms) {
        counters_.backpressure_max_duration_ms = duration;
      }
      blocked_since_ms_ = 0;
    }

    if (actual < n) {
      rewindTail(n - actual);
      break;
    }
    byte_budget -= actual;
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
  last_backpressure_event_ms_ = 0;
  return dropped_bytes;
}

}  // namespace csm::board::uplink
