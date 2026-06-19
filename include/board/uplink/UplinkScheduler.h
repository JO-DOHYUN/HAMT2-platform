#pragma once

#include <stdint.h>

#include "board/uplink/SerialTxScheduler.h"
#include "protocol/TypedFrame.h"

#ifndef BOARD_UPLINK_CRITICAL_QUEUE_RECORDS
#define BOARD_UPLINK_CRITICAL_QUEUE_RECORDS 16
#endif

#ifndef BOARD_UPLINK_NORMAL_QUEUE_RECORDS
#define BOARD_UPLINK_NORMAL_QUEUE_RECORDS 8
#endif

#ifndef BOARD_UPLINK_DIAGNOSTIC_QUEUE_RECORDS
#define BOARD_UPLINK_DIAGNOSTIC_QUEUE_RECORDS 4
#endif

namespace csm::board::uplink {

struct UplinkCounters {
  uint32_t record_accept_total = 0;
  uint32_t record_tx_total = 0;
  uint32_t record_drop_total = 0;
  uint32_t record_encode_fail_total = 0;
  uint32_t critical_drop_total = 0;
  uint32_t normal_drop_total = 0;
  uint32_t diagnostic_drop_total = 0;
  uint32_t critical_queue_high_water = 0;
  uint32_t normal_queue_high_water = 0;
  uint32_t diagnostic_queue_high_water = 0;
  uint32_t queued_payload_high_water_bytes = 0;
};

class UplinkScheduler {
 public:
  void begin(SerialTxScheduler* serial_tx);

  bool enqueueRecord(csm::RecordType type, const uint8_t* payload, uint16_t len,
                     UplinkPriority priority, uint8_t flags = 0);
  SerialTxServiceResult service(uint32_t byte_budget, uint32_t now_ms, uint32_t now_us);

  uint16_t nextTypedSeq() const { return typed_seq_; }
  const UplinkCounters& counters() const { return counters_; }
  bool hasQueuedRecords() const;
  bool hasQueueSpace(UplinkPriority priority) const;
  bool pressureActive() const;
  uint32_t queuedRecordCount() const;
  uint32_t queuedPayloadBytes() const { return queued_payload_bytes_; }
  uint32_t queuedPayloadHighWaterBytes() const {
    return counters_.queued_payload_high_water_bytes;
  }

 private:
  struct PendingRecord {
    csm::RecordType type = static_cast<csm::RecordType>(0);
    uint16_t len = 0;
    uint8_t flags = 0;
    UplinkPriority priority = UplinkPriority::Normal;
    uint8_t payload[csm::kMaxPayloadLen] = {};
  };

  struct RecordQueue {
    PendingRecord* records = nullptr;
    uint8_t capacity = 0;
    uint8_t head = 0;
    uint8_t tail = 0;
    uint8_t count = 0;
  };

  SerialTxScheduler* serial_tx_ = nullptr;
  uint16_t typed_seq_ = 0;
  UplinkCounters counters_;
  uint32_t queued_payload_bytes_ = 0;

  PendingRecord critical_records_[BOARD_UPLINK_CRITICAL_QUEUE_RECORDS] = {};
  PendingRecord normal_records_[BOARD_UPLINK_NORMAL_QUEUE_RECORDS] = {};
  PendingRecord diagnostic_records_[BOARD_UPLINK_DIAGNOSTIC_QUEUE_RECORDS] = {};
  RecordQueue critical_queue_;
  RecordQueue normal_queue_;
  RecordQueue diagnostic_queue_;

  void resetQueues();
  RecordQueue& queueFor(UplinkPriority priority);
  const RecordQueue& queueFor(UplinkPriority priority) const;
  bool push(RecordQueue& queue, const PendingRecord& record);
  bool pop(RecordQueue& queue, PendingRecord& record);
  bool popNext(PendingRecord& record);
  void noteDrop(UplinkPriority priority, bool encode_failure = false);
  void noteQueueHighWater(UplinkPriority priority, uint32_t count);
  void noteQueuedPayloadHighWater();
  bool encodeAndStage(const PendingRecord& record);
  static void accumulate(SerialTxServiceResult& total,
                         const SerialTxServiceResult& next);
};

}  // namespace csm::board::uplink
