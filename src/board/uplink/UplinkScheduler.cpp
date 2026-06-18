#include "board/uplink/UplinkScheduler.h"

namespace csm::board::uplink {

void UplinkScheduler::begin(SerialTxScheduler* serial_tx) {
  serial_tx_ = serial_tx;
  typed_seq_ = 0;
  counters_ = {};
}

bool UplinkScheduler::enqueueRecord(csm::RecordType type, const uint8_t* payload,
                                    uint16_t len, UplinkPriority priority,
                                    uint8_t flags) {
  if (serial_tx_ == nullptr || len > csm::kMaxPayloadLen) {
    counters_.record_drop_total++;
    counters_.record_encode_fail_total++;
    return false;
  }

  const uint32_t frame_len = static_cast<uint32_t>(csm::encoded_typed_frame_len(len));
  if (!serial_tx_->canEnqueue(frame_len, priority)) {
    counters_.record_drop_total++;
    serial_tx_->noteAdmissionFailure(frame_len, priority);
    return false;
  }

  uint8_t frame[2 + 1 + 1 + 1 + 2 + 2 + csm::kMaxPayloadLen + 2];
  size_t written = 0;
  if (!csm::encode_typed_frame(frame, sizeof(frame), type, payload, len, typed_seq_,
                               flags, &written)) {
    counters_.record_drop_total++;
    counters_.record_encode_fail_total++;
    return false;
  }
  typed_seq_++;

  if (!serial_tx_->enqueueAtomic(frame, static_cast<uint32_t>(written), priority)) {
    counters_.record_drop_total++;
    return false;
  }

  counters_.record_tx_total++;
  return true;
}

}  // namespace csm::board::uplink
