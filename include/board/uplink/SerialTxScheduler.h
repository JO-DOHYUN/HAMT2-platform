#pragma once

#include <stddef.h>
#include <stdint.h>

#include "board/uplink/UplinkPriority.h"

namespace csm::board::uplink {

struct SerialTxSchedulerConfig {
  uint32_t critical_reserve_bytes = 0;
  uint32_t drain_time_budget_us = 0;
  uint32_t max_writes_per_pump = 0;
  uint32_t max_bytes_per_pump = 0;
  uint32_t normal_high_water_bytes = 0;
  uint32_t normal_low_water_bytes = 0;
  uint32_t backpressure_event_period_ms = 250;
};

struct SerialTxCounters {
  uint32_t enqueue_fail_total = 0;
  uint32_t reserve_reject_total = 0;
  uint32_t ring_clear_total = 0;
  uint32_t ring_cleared_bytes_total = 0;
  uint32_t backpressure_total = 0;
  uint32_t high_water_bytes = 0;
  uint32_t last_requested_bytes = 0;
  uint32_t last_actual_bytes = 0;
  uint32_t requested_bytes_total = 0;
  uint32_t actual_bytes_total = 0;
  uint32_t actual_zero_total = 0;
  uint32_t partial_write_total = 0;
  uint32_t write_attempt_total = 0;
  uint32_t write_complete_total = 0;
  uint32_t normal_throttle_total = 0;
  uint32_t backpressure_max_duration_ms = 0;
};

struct SerialTxServiceResult {
  uint32_t requested_bytes = 0;
  uint32_t actual_bytes = 0;
  uint32_t writes_attempted = 0;
  uint32_t writes_completed = 0;
  uint32_t partial_writes = 0;
  bool backpressure_event = false;
  uint32_t backpressure_duration_ms = 0;
};

class SerialTxScheduler {
 public:
  void begin(uint8_t* ring, uint32_t ring_size, const SerialTxSchedulerConfig& config);

  uint32_t queuedBytes() const;
  uint32_t freeBytes() const;
  uint32_t capacityBytes() const;
  uint32_t normalLimitBytes() const;
  bool normalThrottleActive() const;
  bool backpressureActive() const;

  bool canEnqueue(uint32_t len, UplinkPriority priority) const;
  void noteAdmissionFailure(uint32_t len, UplinkPriority priority);
  bool enqueueAtomic(const uint8_t* data, uint32_t len, UplinkPriority priority);
  SerialTxServiceResult service(uint32_t byte_budget, uint32_t now_ms, uint32_t now_us);

  uint32_t clearDisconnectedStale();
  const SerialTxCounters& counters() const { return counters_; }

 private:
  uint8_t* ring_ = nullptr;
  uint32_t ring_size_ = 0;
  uint32_t mask_ = 0;
  uint32_t head_ = 0;
  uint32_t tail_ = 0;
  uint32_t blocked_since_ms_ = 0;
  uint32_t last_backpressure_event_ms_ = 0;
  SerialTxSchedulerConfig config_;
  SerialTxCounters counters_;
  bool normal_throttle_active_ = false;

  bool ready() const;
  void updateNormalThrottle();
  void rewindTail(uint32_t count);
};

}  // namespace csm::board::uplink
