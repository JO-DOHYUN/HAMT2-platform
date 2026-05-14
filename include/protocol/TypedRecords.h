#pragma once

#include "protocol/TypedFrame.h"

namespace csm {

enum ControlAckStatus : uint8_t {
  ControlAckRejected = 0,
  ControlAckAccepted = 1,
};

enum ControlAckReason : uint8_t {
  ControlReasonOk = 0,
  ControlReasonBadLength = 1,
  ControlReasonBadBus = 2,
  ControlReasonUnsupportedFrame = 3,
  ControlReasonDlcOutOfRange = 4,
  ControlReasonIdNotAllowed = 5,
  ControlReasonCanNotReady = 6,
  ControlReasonCanWriteFailed = 7,
  ControlReasonBadProtocol = 8,
};

static constexpr uint8_t kControlBus = 1;
static constexpr uint16_t kHostCanTxRequestPayloadLen = 19;
static constexpr uint16_t kCanRawPayloadLen = 30;
static constexpr uint16_t kControlAckPayloadLen = 28;
static constexpr uint16_t kBoardEventPayloadLen = 16;
static constexpr uint16_t kBoardHealthPayloadLen = 52;
static constexpr uint16_t kCapabilityPayloadLen = 36;
static constexpr uint16_t kCapabilityV1PayloadLen = 36;
static constexpr uint16_t kCapabilityV2PayloadLen = 80;
static constexpr uint8_t kCapabilityV2BusDescriptorLen = 20;
static constexpr uint16_t kAdcSamplePayloadLen = 44;

}  // namespace csm
