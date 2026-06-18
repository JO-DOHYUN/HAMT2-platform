#pragma once

#include <stdint.h>

#include "board/uplink/SerialTxScheduler.h"
#include "protocol/TypedFrame.h"

namespace csm::board::uplink {

struct UplinkCounters {
  uint32_t record_tx_total = 0;
  uint32_t record_drop_total = 0;
  uint32_t record_encode_fail_total = 0;
};

class UplinkScheduler {
 public:
  void begin(SerialTxScheduler* serial_tx);

  bool enqueueRecord(csm::RecordType type, const uint8_t* payload, uint16_t len,
                     UplinkPriority priority, uint8_t flags = 0);

  uint16_t nextTypedSeq() const { return typed_seq_; }
  const UplinkCounters& counters() const { return counters_; }

 private:
  SerialTxScheduler* serial_tx_ = nullptr;
  uint16_t typed_seq_ = 0;
  UplinkCounters counters_;
};

}  // namespace csm::board::uplink
