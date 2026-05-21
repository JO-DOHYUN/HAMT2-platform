#include <Arduino.h>
#include <cstring>
#include <mbed.h>
#if defined(SERIAL_CDC)
#include "USB/PluggableUSBSerial.h"
#endif

#include "BoardPins.h"
#include "board/CapabilityPublisher.h"
#include "board/HostDownlinkParser.h"
#include "board/SafetySupervisor.h"
#include "board/StatusLed.h"
#include "protocol/HostCommands.h"
#include "protocol/TypedFrame.h"
#include "protocol/TypedRecords.h"

#ifndef BOARD_HW_PROFILE_MID_TJA1051_DUAL
#define BOARD_HW_PROFILE_MID_TJA1051_DUAL 0
#endif

#ifndef BOARD_HW_PROFILE_MID_MCP2515
#define BOARD_HW_PROFILE_MID_MCP2515 0
#endif

#ifndef BOARD_TARGET_INTERNAL_CAN_LANE0
#define BOARD_TARGET_INTERNAL_CAN_LANE0 0
#endif

#ifndef BOARD_TARGET_INTERNAL_CAN_LANE1
#define BOARD_TARGET_INTERNAL_CAN_LANE1 0
#endif

#ifndef BOARD_ENABLE_MCP2515
#define BOARD_ENABLE_MCP2515 1
#endif

#ifndef BOARD_ENABLE_MCP2515_INIT
#define BOARD_ENABLE_MCP2515_INIT BOARD_ENABLE_MCP2515
#endif

#ifndef BOARD_ENABLE_TIM3_ENCODER
#define BOARD_ENABLE_TIM3_ENCODER 0
#endif

#ifndef BOARD_ENABLE_SAFETY_IO
#define BOARD_ENABLE_SAFETY_IO 1
#endif

#ifndef BOARD_ENABLE_ENCODER_IO
#define BOARD_ENABLE_ENCODER_IO 1
#endif

#ifndef BOARD_CAN_USE_INT
#define BOARD_CAN_USE_INT 0
#endif

// 0: periodic polling only
// 1: MCP2515 INT_N active-low level hint + bounded polling fallback
// 2: EXTI falling-edge hint + INT_N active-low level + bounded polling fallback
#ifndef BOARD_CAN_IRQ_MODE
#if BOARD_CAN_USE_INT
#define BOARD_CAN_IRQ_MODE 1
#else
#define BOARD_CAN_IRQ_MODE 0
#endif
#endif

#ifndef BOARD_CAN_POLL_FALLBACK_US
#define BOARD_CAN_POLL_FALLBACK_US 2000
#endif

#ifndef BOARD_CAN_SERIAL_DRAIN_BUDGET
#define BOARD_CAN_SERIAL_DRAIN_BUDGET 32
#endif

#ifndef BOARD_CAN_ERROR_EVENT_PERIOD_MS
#define BOARD_CAN_ERROR_EVENT_PERIOD_MS 500
#endif

#ifndef BOARD_CAPABILITY_PERIOD_MS
#define BOARD_CAPABILITY_PERIOD_MS 2000
#endif

#ifndef BOARD_ENABLE_STATUS_LED
#define BOARD_ENABLE_STATUS_LED 1
#endif

#ifndef BOARD_ENABLE_RUNTIME_WATCHDOG
#define BOARD_ENABLE_RUNTIME_WATCHDOG 1
#endif

#ifndef BOARD_RUNTIME_WATCHDOG_TIMEOUT_MS
#define BOARD_RUNTIME_WATCHDOG_TIMEOUT_MS 3000
#endif

#ifndef BOARD_USB_CDC_RECONNECT_RESET_MS
#define BOARD_USB_CDC_RECONNECT_RESET_MS 1500
#endif

#ifndef BOARD_ENABLE_MCP2515_TX_TEST
#define BOARD_ENABLE_MCP2515_TX_TEST 0
#endif

#ifndef BOARD_MCP2515_TX_PERIOD_MS
#define BOARD_MCP2515_TX_PERIOD_MS 500
#endif

#ifndef BOARD_MCP2515_TX_TEST_ID
#define BOARD_MCP2515_TX_TEST_ID 0x321
#endif

#ifndef BOARD_MCP2515_TX_AUDIT_TIMEOUT_MS
#define BOARD_MCP2515_TX_AUDIT_TIMEOUT_MS 100
#endif

#ifndef BOARD_MCP2515_TX_USE_ONESHOT
#define BOARD_MCP2515_TX_USE_ONESHOT 0
#endif

#ifndef BOARD_ENABLE_BUILTIN_CAN_TX_TEST
#define BOARD_ENABLE_BUILTIN_CAN_TX_TEST 0
#endif

#ifndef BOARD_ENABLE_BUILTIN_CAN_RX
#define BOARD_ENABLE_BUILTIN_CAN_RX BOARD_ENABLE_BUILTIN_CAN_TX_TEST
#endif

#ifndef BOARD_ENABLE_HOST_CAN_TX
#define BOARD_ENABLE_HOST_CAN_TX BOARD_ENABLE_BUILTIN_CAN_TX_TEST
#endif

#ifndef BOARD_ENABLE_HOST_CAN_TX_BUILTIN
#define BOARD_ENABLE_HOST_CAN_TX_BUILTIN BOARD_ENABLE_HOST_CAN_TX
#endif

#ifndef BOARD_ENABLE_HOST_CAN_TX_MCP2515
#define BOARD_ENABLE_HOST_CAN_TX_MCP2515 0
#endif

#define BOARD_ENABLE_HOST_CAN_TX_ANY (BOARD_ENABLE_HOST_CAN_TX || BOARD_ENABLE_HOST_CAN_TX_BUILTIN || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
#define BOARD_ENABLE_BUILTIN_CAN_LANE (BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_BUILTIN_CAN_RX || BOARD_ENABLE_HOST_CAN_TX_BUILTIN)

#ifndef BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
#define BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND 0
#endif

#ifndef BOARD_BUILTIN_CAN_RX_BUS
#define BOARD_BUILTIN_CAN_RX_BUS 1
#endif

#ifndef BOARD_MCP2515_BUS_ID
#define BOARD_MCP2515_BUS_ID 0
#endif

#ifndef BOARD_MCP2515_BUS_ROLE
#define BOARD_MCP2515_BUS_ROLE 2
#endif

#ifndef BOARD_BUILTIN_CAN_BUS_ID
#define BOARD_BUILTIN_CAN_BUS_ID BOARD_BUILTIN_CAN_RX_BUS
#endif

#ifndef BOARD_BUILTIN_CAN_BUS_ROLE
#define BOARD_BUILTIN_CAN_BUS_ROLE 1
#endif

#ifndef BOARD_MCP2515_CONTROL_TX_ALLOWED
#define BOARD_MCP2515_CONTROL_TX_ALLOWED BOARD_ENABLE_HOST_CAN_TX_MCP2515
#endif

#ifndef BOARD_BUILTIN_CAN_CONTROL_TX_ALLOWED
#define BOARD_BUILTIN_CAN_CONTROL_TX_ALLOWED BOARD_ENABLE_HOST_CAN_TX_BUILTIN
#endif

#ifndef BOARD_ENABLE_VOLTAGE_ADC
#define BOARD_ENABLE_VOLTAGE_ADC 0
#endif

#ifndef BOARD_VOLTAGE_SAMPLE_PERIOD_MS
#define BOARD_VOLTAGE_SAMPLE_PERIOD_MS 20
#endif

#ifndef BOARD_BUILTIN_CAN_TX_PERIOD_MS
#define BOARD_BUILTIN_CAN_TX_PERIOD_MS 500
#endif

#ifndef BOARD_BUILTIN_CAN_TX_TEST_ID
#define BOARD_BUILTIN_CAN_TX_TEST_ID 0x322
#endif

#ifndef BOARD_ENABLE_CAN_TX_GATE_FOR_TEST
#define BOARD_ENABLE_CAN_TX_GATE_FOR_TEST BOARD_ENABLE_BUILTIN_CAN_TX_TEST
#endif

#if BOARD_ENABLE_BUILTIN_CAN_LANE
#include <Arduino_CAN.h>
#endif

#if BOARD_ENABLE_MCP2515
#include <SPI.h>
#include <mcp2515.h>
#endif

// Build-time mode switch. Keep false for real CAN capture.
static constexpr bool kTestMode = false;

#if BOARD_ENABLE_MCP2515
static constexpr CAN_SPEED kMcp2515Bitrate = CAN_500KBPS;
static constexpr CAN_CLOCK kMcp2515Clock = MCP_8MHZ;
#ifndef BOARD_MCP2515_SPI_HZ
#define BOARD_MCP2515_SPI_HZ 250000
#endif
static constexpr uint32_t kMcp2515SpiHz = BOARD_MCP2515_SPI_HZ;
#endif

using csm::crc16_ccitt;
using csm::kFrameSof0;
using csm::kFrameSof1;
using csm::kHostCanTxAllowedPrimaryId;
using csm::kHostCanTxAllowedRangeEnd;
using csm::kHostCanTxAllowedRangeStart;
using csm::kMaxPayloadLen;
using csm::kProtocolVersion;
using csm::kCapabilityV1PayloadLen;
using csm::kCapabilityV2BusDescriptorLen;
using csm::kCapabilityV2PayloadLen;
using csm::kCapabilityV3PayloadLen;
using csm::kBoardHealthV2PayloadLen;
using csm::mono64_us;
using csm::rd_u16_le;
using csm::rd_u32_le;
using csm::RecordType;
using csm::wr_i32_le;
using csm::wr_i64_le;
using csm::wr_u16_le;
using csm::wr_u32_le;
using csm::wr_u64_le;

using SafetyState = csm::board::SafetyState;

enum BoardEventCode : uint16_t {
  EventBoot = 1,
  EventCanBeginFailed = 2,
  EventCanRxQueueDrop = 3,
  EventEncoderFaultAsserted = 4,
  EventFieldPowerLost = 5,
  EventEstopAsserted = 6,
  EventEncoderIndex = 7,
  EventEncoderWrap = 8,
  EventMcp2515Error = 9,
  EventMcp2515SpiSnapshot = 10,
  EventBuiltinCanBeginFailed = 11,
  EventBuiltinCanTxFailed = 12,
  EventHostFrameCrcFailed = 13,
  EventHostCanTxRejected = 14,
  EventHostCanTxAccepted = 15,
  EventCan0BackendUnavailable = 16,
  EventMcp2515TxFailed = 17,
  EventSafetyStateChanged = 18,
  EventHostHeartbeat = 19,
  EventHostControlSession = 20,
  EventHostCommandUnsupported = 21,
  EventFaultLockoutCleared = 22,
};

struct CanRxItem {
  uint64_t mono_us;
  uint32_t can_id_flags;
  uint8_t dlc_flags;
  uint8_t bus;
  uint8_t data[8];
};

struct EncoderSnapshot {
  uint64_t mono_us;
  int64_t position;
  uint16_t tim3_count;
  uint8_t ab_state;
  uint8_t flags;
  uint32_t fault_flags;
};

static constexpr uint32_t kCanQueueSize = 1024;
static constexpr uint32_t kCanQueueMask = kCanQueueSize - 1;

static constexpr uint8_t kVoltageAdcBits = 12;
static constexpr uint8_t kVoltageChannelCount = 4;
#if BOARD_ENABLE_VOLTAGE_ADC
static constexpr pin_size_t kVoltagePins[kVoltageChannelCount] = {
    BoardPins::VoltageSense0,
    BoardPins::VoltageSense1,
    BoardPins::VoltageSense2,
    BoardPins::VoltageSense3,
};
static constexpr uint8_t kVoltageChannelIds[kVoltageChannelCount] = {0, 1, 2, 3};
#endif

static CanRxItem can_queue[kCanQueueSize];
static volatile uint32_t can_q_head = 0;
static volatile uint32_t can_q_tail = 0;

static volatile uint32_t can_rx_count_total = 0;
static volatile uint32_t can_rx_dropped_total = 0;
static volatile uint32_t can_fifo_overflow_total = 0;
static volatile uint32_t serial_record_tx_total = 0;
static volatile uint32_t serial_record_drop_total = 0;
static uint32_t host_frame_crc_failed_total = 0;
static uint32_t host_heartbeat_total = 0;
static uint32_t host_control_session_total = 0;
static uint32_t host_can_tx_request_total = 0;
static uint32_t __attribute__((unused)) host_can_tx_accepted_total = 0;
static uint32_t host_can_tx_rejected_total = 0;

static volatile bool encoder_index_pending = false;
static volatile uint64_t encoder_index_mono_us = 0;
static volatile uint32_t encoder_index_count = 0;

static uint16_t transport_seq = 0;
static constexpr uint32_t kSerialTxRingSize = 8192;
static constexpr uint32_t kSerialTxRingMask = kSerialTxRingSize - 1;
static uint8_t serial_tx_ring[kSerialTxRingSize];
static uint32_t serial_tx_head = 0;
static uint32_t serial_tx_tail = 0;
static uint32_t serial_tx_blocked_since_ms = 0;
#if BOARD_ENABLE_MCP2515
static MCP2515* mcp2515 = nullptr;
static volatile bool mcp2515_irq_pending = false;
static volatile uint32_t mcp2515_irq_edges = 0;
#endif
static bool can_backend_ok = false;
static bool voltage_adc_ok = false;
#if BOARD_ENABLE_VOLTAGE_ADC
static uint32_t voltage_adc_sample_total = 0;
static uint32_t voltage_adc_drop_total = 0;
static uint32_t last_voltage_adc_sample_ms = 0;
#endif

#if BOARD_ENABLE_MCP2515
struct Mcp2515ServiceState {
  bool int_pin_enabled;
  bool exti_attached;
  bool last_int_low;
  uint32_t last_probe_us;
  uint32_t last_housekeeping_ms;
  uint32_t last_error_event_ms;
  uint32_t last_seen_irq_edges;
  uint32_t int_low_samples;
  uint32_t nomsg_while_int_low;
  uint32_t spi_error_total;
  uint32_t error_flag_total;
  uint8_t last_canintf;
  uint8_t last_eflg;
  uint8_t last_canctrl;
};

static Mcp2515ServiceState mcp_service = {};
#endif

#if BOARD_ENABLE_BUILTIN_CAN_LANE
static bool builtin_can_tx_ok = false;
#endif

#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_BUILTIN
static uint32_t builtin_can_tx_total = 0;
static uint32_t builtin_can_tx_failed_total = 0;
#endif

#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST
static uint32_t last_builtin_can_tx_ms = 0;
#endif

#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
static uint32_t mcp2515_tx_total = 0;
static uint32_t mcp2515_tx_failed_total = 0;
static uint32_t mcp2515_tx_queue_full_total = 0;
struct Mcp2515PendingTx {
  bool active;
  uint32_t start_ms;
  uint8_t bus;
  uint32_t can_id_flags;
  uint8_t dlc;
  uint8_t data[8];
};
static Mcp2515PendingTx mcp2515_pending_tx = {};
static constexpr uint32_t kMcp2515TxQueueSize = 256;
static constexpr uint32_t kMcp2515TxQueueMask = kMcp2515TxQueueSize - 1;
static Mcp2515PendingTx mcp2515_tx_queue[kMcp2515TxQueueSize];
static uint32_t mcp2515_tx_q_head = 0;
static uint32_t mcp2515_tx_q_tail = 0;
#endif

#if BOARD_ENABLE_MCP2515 && BOARD_ENABLE_MCP2515_TX_TEST
static uint32_t last_mcp2515_tx_ms = 0;
#endif

#if BOARD_ENABLE_STATUS_LED
static csm::board::StatusLed status_led;
#endif

static TIM_HandleTypeDef htim3;
static bool encoder_timer_ok = false;
static uint16_t encoder_last_count = 0;
static int64_t encoder_position = 0;
static uint32_t encoder_wrap_events = 0;
static uint32_t encoder_fault_events = 0;
static bool encoder_fault_prev = false;
static bool field_power_prev = true;
static bool estop_prev = false;
static csm::board::SafetySupervisor safety_supervisor;
static SafetyState safety_state = SafetyState::MonitorOnly;

static uint32_t last_health_ms = 0;
static uint32_t last_capability_ms = 0;
static uint32_t last_encoder_derived_ms = 0;
static uint32_t last_watchdog_toggle_ms = 0;
static uint32_t last_can_init_retry_ms = 0;
static bool usb_host_was_connected = false;
static uint32_t usb_disconnected_since_ms = 0;

static uint32_t serial_tx_queued_bytes() {
  return (serial_tx_head - serial_tx_tail) & kSerialTxRingMask;
}

static uint32_t serial_tx_free_bytes() {
  return (kSerialTxRingSize - 1) - serial_tx_queued_bytes();
}

static void clear_serial_tx_ring() {
  serial_tx_head = 0;
  serial_tx_tail = 0;
  serial_tx_blocked_since_ms = 0;
}

static bool enqueue_serial_tx_bytes(const uint8_t* data, uint32_t len) {
  if (len == 0) return true;
  if (data == nullptr || len > serial_tx_free_bytes()) return false;
  for (uint32_t i = 0; i < len; ++i) {
    serial_tx_ring[serial_tx_head] = data[i];
    serial_tx_head = (serial_tx_head + 1) & kSerialTxRingMask;
  }
  return true;
}

static void service_serial_tx(uint32_t byte_budget = 512) {
#if defined(SERIAL_CDC)
  uint8_t chunk[64];
  while (byte_budget > 0 && serial_tx_tail != serial_tx_head) {
    uint32_t n = 0;
    while (n < sizeof(chunk) && n < byte_budget && serial_tx_tail != serial_tx_head) {
      chunk[n++] = serial_tx_ring[serial_tx_tail];
      serial_tx_tail = (serial_tx_tail + 1) & kSerialTxRingMask;
    }
    if (n == 0) break;

    uint32_t actual = 0;
    _SerialUSB.send_nb(chunk, n, &actual, true);
    if (actual == 0) {
      for (uint32_t i = actual; i < n; ++i) {
        serial_tx_tail = (serial_tx_tail - 1) & kSerialTxRingMask;
      }
      const uint32_t now_ms = millis();
      if (serial_tx_blocked_since_ms == 0) {
        serial_tx_blocked_since_ms = now_ms;
      } else if (now_ms - serial_tx_blocked_since_ms > 250) {
        clear_serial_tx_ring();
      }
      break;
    }
    serial_tx_blocked_since_ms = 0;
    if (actual < n) {
      for (uint32_t i = actual; i < n; ++i) {
        serial_tx_tail = (serial_tx_tail - 1) & kSerialTxRingMask;
      }
      break;
    }
    byte_budget -= actual;
  }
#else
  (void)byte_budget;
#endif
}

static void service_usb_cdc_reconnect_watchdog() {
#if defined(SERIAL_CDC)
  const bool connected = _SerialUSB.connected();
  if (connected) {
    usb_host_was_connected = true;
    usb_disconnected_since_ms = 0;
    return;
  }

  if (!usb_host_was_connected) {
    return;
  }

  const uint32_t now_ms = millis();
  if (usb_disconnected_since_ms == 0) {
    usb_disconnected_since_ms = now_ms;
    clear_serial_tx_ring();
    return;
  }

  if (now_ms - usb_disconnected_since_ms >= BOARD_USB_CDC_RECONNECT_RESET_MS) {
    delay(10);
    NVIC_SystemReset();
  }
#endif
}

static bool enqueue_typed_record(RecordType type, const uint8_t* payload, uint16_t len, uint8_t flags = 0) {
  if (len > kMaxPayloadLen) {
    return false;
  }

#if !defined(SERIAL_CDC)
  return csm::emit_typed_record(Serial, type, payload, len, transport_seq, flags);
#else
  uint8_t frame[2 + 1 + 1 + 1 + 2 + 2 + kMaxPayloadLen + 2];
  size_t pos = 0;
  frame[pos++] = kFrameSof0;
  frame[pos++] = kFrameSof1;
  frame[pos++] = kProtocolVersion;
  frame[pos++] = static_cast<uint8_t>(type);
  frame[pos++] = flags;
  wr_u16_le(&frame[pos], transport_seq++);
  pos += 2;
  wr_u16_le(&frame[pos], len);
  pos += 2;
  if (len > 0 && payload != nullptr) {
    memcpy(&frame[pos], payload, len);
    pos += len;
  }

  const uint16_t crc = crc16_ccitt(&frame[2], pos - 2);
  wr_u16_le(&frame[pos], crc);
  pos += 2;

  if (!enqueue_serial_tx_bytes(frame, static_cast<uint32_t>(pos))) {
    return false;
  }
  service_serial_tx(512);
  return true;
#endif
}

static void emit_record(RecordType type, const uint8_t* payload, uint16_t len, uint8_t flags = 0) {
  if (enqueue_typed_record(type, payload, len, flags)) {
    serial_record_tx_total++;
  } else {
    serial_record_drop_total++;
  }
}

#if BOARD_ENABLE_STATUS_LED
static void init_status_led() {
  status_led.begin();
}

static void service_status_led() {
  status_led.service(can_backend_ok);
}
#else
static void init_status_led() {}
static void service_status_led() {}
#endif

static void init_runtime_watchdog() {
#if BOARD_ENABLE_RUNTIME_WATCHDOG
  mbed::Watchdog::get_instance().start(BOARD_RUNTIME_WATCHDOG_TIMEOUT_MS);
#endif
}

static void kick_runtime_watchdog() {
#if BOARD_ENABLE_RUNTIME_WATCHDOG
  mbed::Watchdog::get_instance().kick();
#endif
}

static void emit_board_event(uint16_t code, uint16_t detail, uint32_t counter) {
  uint8_t payload[16];
  wr_u64_le(&payload[0], mono64_us());
  wr_u16_le(&payload[8], code);
  wr_u16_le(&payload[10], detail);
  wr_u32_le(&payload[12], counter);
  emit_record(RecordType::BoardEvent, payload, sizeof(payload));
}

using csm::ControlAckAccepted;
using csm::ControlAckRejected;
using csm::ControlReasonBadBus;
using csm::ControlReasonBadLength;
using csm::ControlReasonBadProtocol;
using csm::ControlReasonCanNotReady;
using csm::ControlReasonCanWriteFailed;
using csm::ControlReasonControlLeaseExpired;
using csm::ControlReasonDlcOutOfRange;
using csm::ControlReasonEstopAsserted;
using csm::ControlReasonFieldPowerLost;
using csm::ControlReasonIdNotAllowed;
using csm::ControlReasonHostTimeout;
using csm::ControlReasonNotArmed;
using csm::ControlReasonOk;
using csm::ControlReasonQueueFull;
using csm::ControlReasonSafetyLockout;
using csm::ControlReasonUnsupportedFrame;
using csm::ControlReasonUnsupportedCommand;

static void emit_control_ack(uint32_t command_id, uint8_t status, uint8_t reason, uint8_t bus,
                             uint32_t can_id_flags, uint8_t dlc, uint32_t counter) {
  uint8_t payload[28];
  memset(payload, 0, sizeof(payload));
  wr_u64_le(&payload[0], mono64_us());
  wr_u32_le(&payload[8], command_id);
  payload[12] = status;
  payload[13] = reason;
  payload[14] = bus;
  payload[15] = dlc & 0x0F;
  wr_u32_le(&payload[16], can_id_flags);
  wr_u32_le(&payload[20], counter);
  wr_u32_le(&payload[24], host_can_tx_rejected_total);
  emit_record(RecordType::ControlAck, payload, sizeof(payload));
}

#if BOARD_HW_PROFILE_MID_MCP2515 || BOARD_HW_PROFILE_MID_TJA1051_DUAL
static csm::board::CapabilityBusDescriptor make_capability_bus_descriptor(
    uint8_t bus_id, uint8_t role, uint8_t backend, uint8_t transceiver,
    uint8_t rx_supported, uint8_t tx_supported, uint8_t control_tx_allowed,
    uint8_t termination_policy, uint8_t isolation_policy) {
  csm::board::CapabilityBusDescriptor bus;
  bus.bus_id = bus_id;
  bus.role = role;
  bus.backend = backend;
  bus.transceiver = transceiver;
  bus.rx_supported = rx_supported;
  bus.tx_supported = tx_supported;
  bus.control_tx_allowed = control_tx_allowed;
  bus.termination_policy = termination_policy;
  bus.isolation_policy = isolation_policy;
  return bus;
}
#endif

static void emit_capability() {
  csm::board::CapabilityPayloadConfig config;
#if BOARD_HW_PROFILE_MID_MCP2515
  config.profile_major = 3;   // Mid Carrier + external MCP2515 current CSM
#elif BOARD_HW_PROFILE_MID_TJA1051_DUAL
  config.profile_major = 2;   // Mid Carrier + TJA1051 target
#else
  config.profile_major = 1;
#endif
  config.profile_minor = 0;
  config.can_queue_size = kCanQueueSize;
  config.encoder_ppr = 2048;
  config.encoder_frequency_limit_hz = 300000;
  config.adc_sample_supported = BOARD_ENABLE_VOLTAGE_ADC;
  config.adc_channel_count = BOARD_ENABLE_VOLTAGE_ADC ? kVoltageChannelCount : 0;
  config.adc_resolution_bits = BOARD_ENABLE_VOLTAGE_ADC ? kVoltageAdcBits : 0;
  config.adc_sample_period_ms = static_cast<uint8_t>(BOARD_VOLTAGE_SAMPLE_PERIOD_MS & 0xFF);

  uint8_t lane_flags = 0;
#if BOARD_ENABLE_MCP2515
  lane_flags |= (1u << 0);
  lane_flags |= (BOARD_CAN_IRQ_MODE != 0) ? (1u << 4) : 0;
#endif
#if BOARD_ENABLE_BUILTIN_CAN_RX
  lane_flags |= (1u << 1);
#endif
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  lane_flags |= (1u << 2);
#endif
#if BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  lane_flags |= (1u << 3);
#endif
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  lane_flags |= (1u << 6);
#endif
#if BOARD_ENABLE_HOST_CAN_TX_MCP2515
  lane_flags |= (1u << 7);
#endif
#if BOARD_ENABLE_VOLTAGE_ADC
  lane_flags |= (1u << 5);
#endif
  config.lane_capability_flags = lane_flags;

  uint8_t limitation_flags = 0;
#if BOARD_ENABLE_BUILTIN_CAN_LANE
  limitation_flags |= (1u << 0);
#endif
#if BOARD_ENABLE_HOST_CAN_TX_ANY
  limitation_flags |= (1u << 1);
#endif
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_MCP2515_TX_TEST
  limitation_flags |= (1u << 2);
#endif
  config.limitation_flags = limitation_flags;

#if BOARD_HW_PROFILE_MID_MCP2515 || BOARD_HW_PROFILE_MID_TJA1051_DUAL
  config.include_v2 = true;
  config.include_v3 = true;
#if BOARD_HW_PROFILE_MID_MCP2515
  config.bus_count = BOARD_ENABLE_BUILTIN_CAN_LANE ? 2 : 1;
#else
  config.bus_count = 2;
#endif
  config.capability_v2_flags = 0x0003;
  config.supported_uplink_records =
      (1u << static_cast<uint8_t>(RecordType::CanRxRaw)) |
      (1u << static_cast<uint8_t>(RecordType::CanTxRaw)) |
      (1u << static_cast<uint8_t>(RecordType::AdcSample)) |
      (1u << static_cast<uint8_t>(RecordType::ControlAck)) |
      (1u << static_cast<uint8_t>(RecordType::BoardEvent)) |
      (1u << static_cast<uint8_t>(RecordType::BoardHealth)) |
      (1u << static_cast<uint8_t>(RecordType::Capability));
  config.supported_downlink_records =
      (1u << static_cast<uint8_t>(RecordType::HostCanTxRequest)) |
      (1u << static_cast<uint8_t>(RecordType::HostHeartbeat)) |
      (1u << static_cast<uint8_t>(RecordType::HostControlSession)) |
      (1u << static_cast<uint8_t>(RecordType::HostQueryCapability)) |
      (1u << static_cast<uint8_t>(RecordType::HostClearFaultLockout));
  config.safety_feature_flags = 0x0000000Fu;
  config.host_tx_queue_size = 32;
  config.capability_v3_flags = 0x0001;
#if BOARD_TARGET_INTERNAL_CAN_LANE0 && !BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
  config.capability_v2_flags |= (1u << 2);
#endif

#if BOARD_HW_PROFILE_MID_MCP2515
  config.buses[0] = make_capability_bus_descriptor(
      BOARD_MCP2515_BUS_ID,
      0,
      1,
      1,
      BOARD_ENABLE_MCP2515 ? 1 : 0,
      (BOARD_ENABLE_HOST_CAN_TX_MCP2515 || BOARD_ENABLE_MCP2515_TX_TEST) ? 1 : 0,
      BOARD_MCP2515_CONTROL_TX_ALLOWED ? 1 : 0,
      3,
      1);
#if BOARD_ENABLE_BUILTIN_CAN_LANE
  config.buses[1] = make_capability_bus_descriptor(
      BOARD_BUILTIN_CAN_BUS_ID,
      0,
      2,
      3,
      BOARD_ENABLE_BUILTIN_CAN_RX ? 1 : 0,
      (BOARD_ENABLE_HOST_CAN_TX_BUILTIN || BOARD_ENABLE_BUILTIN_CAN_TX_TEST) ? 1 : 0,
      BOARD_BUILTIN_CAN_CONTROL_TX_ALLOWED ? 1 : 0,
      0,
      0);
#endif
#else
  config.buses[0] = make_capability_bus_descriptor(
      0,
      0,
#if BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
      3,
      2,
      1,
      1,
#else
      4,
      2,
      0,
      0,
#endif
      0,
      2,
      1);

  config.buses[1] = make_capability_bus_descriptor(
      BOARD_BUILTIN_CAN_BUS_ID,
      0,
      2,
      3,
      BOARD_ENABLE_BUILTIN_CAN_RX ? 1 : 0,
      (BOARD_ENABLE_HOST_CAN_TX || BOARD_ENABLE_BUILTIN_CAN_TX_TEST) ? 1 : 0,
      BOARD_BUILTIN_CAN_CONTROL_TX_ALLOWED ? 1 : 0,
      0,
      0);
#endif
#endif

  uint8_t payload[kCapabilityV3PayloadLen];
  const uint16_t payload_len = csm::board::build_capability_payload(config, payload, sizeof(payload));
  if (payload_len > 0) {
    emit_record(RecordType::Capability, payload, payload_len);
  }
}

static void service_capability_advertisement() {
  const uint32_t now_ms = millis();
  if (now_ms - last_capability_ms < BOARD_CAPABILITY_PERIOD_MS) {
    return;
  }
  last_capability_ms = now_ms;
  emit_capability();
}

static bool can_queue_push(const CanRxItem& item) {
  const uint32_t head = can_q_head;
  const uint32_t next = (head + 1) & kCanQueueMask;
  if (next == can_q_tail) {
    can_rx_dropped_total++;
    if ((can_rx_dropped_total & 0xFF) == 1) {
      emit_board_event(EventCanRxQueueDrop, 0, can_rx_dropped_total);
    }
    return false;
  }
  can_queue[head] = item;
  can_q_head = next;
  return true;
}

static bool can_queue_pop(CanRxItem& out) {
  const uint32_t tail = can_q_tail;
  if (tail == can_q_head) {
    return false;
  }
  out = can_queue[tail];
  can_q_tail = (tail + 1) & kCanQueueMask;
  return true;
}

static uint8_t read_ab_state() {
#if BOARD_ENABLE_ENCODER_IO
  const uint8_t a = digitalRead(BoardPins::EncoderA) ? 1 : 0;
  const uint8_t b = digitalRead(BoardPins::EncoderB) ? 1 : 0;
  return static_cast<uint8_t>((b << 1) | a);
#else
  return 0;
#endif
}

static uint32_t read_fault_flags() {
  uint32_t flags = 0;
#if BOARD_ENABLE_SAFETY_IO
  if (!digitalRead(BoardPins::EncoderFaultN)) {
    flags |= (1u << 0);
  }
  if (!digitalRead(BoardPins::FieldPowerOk)) {
    flags |= (1u << 1);
  }
  if (!digitalRead(BoardPins::EstopInN)) {
    flags |= (1u << 2);
  }
#endif
  return flags;
}

void on_encoder_index() {
  encoder_index_mono_us = mono64_us();
  encoder_index_count++;
  encoder_index_pending = true;
}

#if BOARD_ENABLE_MCP2515 && (BOARD_CAN_IRQ_MODE == 2)
void on_mcp2515_int() {
  mcp2515_irq_edges++;
  mcp2515_irq_pending = true;
}
#endif

static bool init_encoder_timer3() {
#if BOARD_ENABLE_TIM3_ENCODER
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {};
  gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOC, &gpio);

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 0xFFFF;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  TIM_Encoder_InitTypeDef enc = {};
  enc.EncoderMode = TIM_ENCODERMODE_TI12;
  enc.IC1Polarity = TIM_ICPOLARITY_RISING;
  enc.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  enc.IC1Prescaler = TIM_ICPSC_DIV1;
  enc.IC1Filter = 4;
  enc.IC2Polarity = TIM_ICPOLARITY_RISING;
  enc.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  enc.IC2Prescaler = TIM_ICPSC_DIV1;
  enc.IC2Filter = 4;

  if (HAL_TIM_Encoder_Init(&htim3, &enc) != HAL_OK) {
    return false;
  }
  if (HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK) {
    return false;
  }
  encoder_last_count = static_cast<uint16_t>(__HAL_TIM_GET_COUNTER(&htim3));
  return true;
#else
  encoder_last_count = 0;
  return false;
#endif
}

static EncoderSnapshot poll_encoder() {
  EncoderSnapshot snap;
  snap.mono_us = mono64_us();
  snap.tim3_count = encoder_timer_ok ? static_cast<uint16_t>(__HAL_TIM_GET_COUNTER(&htim3)) : 0;
  const int16_t delta = static_cast<int16_t>(snap.tim3_count - encoder_last_count);
  encoder_last_count = snap.tim3_count;
  encoder_position += delta;
  if (delta > 30000 || delta < -30000) {
    encoder_wrap_events++;
  }
  snap.position = encoder_position;
  snap.ab_state = read_ab_state();
  snap.flags = 0;
  snap.fault_flags = read_fault_flags();
  return snap;
}

static void emit_encoder_edge(const EncoderSnapshot& snap, uint8_t flags) {
  uint8_t payload[28];
  wr_u64_le(&payload[0], snap.mono_us);
  wr_i64_le(&payload[8], snap.position);
  wr_u16_le(&payload[16], snap.tim3_count);
  payload[18] = snap.ab_state;
  payload[19] = flags;
  wr_u32_le(&payload[20], snap.fault_flags);
  wr_u32_le(&payload[24], encoder_index_count);
  emit_record(RecordType::EncEdgeRaw, payload, sizeof(payload));
}

static void emit_encoder_derived(const EncoderSnapshot& snap, int32_t velocity_counts_per_s) {
  uint8_t payload[28];
  wr_u64_le(&payload[0], snap.mono_us);
  wr_i64_le(&payload[8], snap.position);
  wr_i32_le(&payload[16], velocity_counts_per_s);
  wr_u16_le(&payload[20], snap.tim3_count);
  payload[22] = snap.ab_state;
  payload[23] = snap.fault_flags ? 1 : 0;
  wr_u32_le(&payload[24], snap.fault_flags);
  emit_record(RecordType::EncDerived, payload, sizeof(payload));
}

static void emit_can_rx_raw(const CanRxItem& item) {
  uint8_t payload[30];
  wr_u64_le(&payload[0], item.mono_us);
  wr_u32_le(&payload[8], item.can_id_flags);
  payload[12] = item.dlc_flags;
  payload[13] = item.bus;
  memcpy(&payload[14], item.data, 8);
  wr_u32_le(&payload[22], can_rx_count_total);
  wr_u32_le(&payload[26], can_rx_dropped_total);
  emit_record(RecordType::CanRxRaw, payload, sizeof(payload));
}

static void __attribute__((unused)) emit_can_tx_raw(uint8_t bus, uint32_t can_id_flags, uint8_t dlc,
                                                    const uint8_t* data, uint32_t tx_total,
                                                    uint32_t tx_failed_total) {
  uint8_t payload[30];
  memset(payload, 0, sizeof(payload));
  wr_u64_le(&payload[0], mono64_us());
  wr_u32_le(&payload[8], can_id_flags);
  payload[12] = dlc & 0x0F;
  payload[13] = bus;
  if (data != nullptr) {
    memcpy(&payload[14], data, dlc > 8 ? 8 : dlc);
  }
  wr_u32_le(&payload[22], tx_total);
  wr_u32_le(&payload[26], tx_failed_total);
  emit_record(RecordType::CanTxRaw, payload, sizeof(payload));
}

static void emit_board_health(const EncoderSnapshot& snap) {
  const uint32_t head = can_q_head;
  const uint32_t tail = can_q_tail;
  const uint32_t queued = (head - tail) & kCanQueueMask;

  uint8_t inputs = 0;
#if BOARD_ENABLE_SAFETY_IO
  inputs |= digitalRead(BoardPins::EstopInN) ? 0 : (1u << 0);
  inputs |= digitalRead(BoardPins::FieldPowerOk) ? (1u << 1) : 0;
  inputs |= digitalRead(BoardPins::EncoderFaultN) ? 0 : (1u << 2);
  inputs |= digitalRead(BoardPins::ArmKeyIn) ? (1u << 3) : 0;
#endif

  uint8_t payload[kBoardHealthV2PayloadLen];
  memset(payload, 0, sizeof(payload));
  wr_u64_le(&payload[0], mono64_us());
  wr_u32_le(&payload[8], can_rx_count_total);
  wr_u32_le(&payload[12], can_rx_dropped_total);
  wr_u32_le(&payload[16], can_fifo_overflow_total);
  wr_u32_le(&payload[20], serial_record_tx_total);
  wr_u32_le(&payload[24], queued);
  wr_u32_le(&payload[28], encoder_fault_events);
  wr_u32_le(&payload[32], encoder_wrap_events);
  wr_i64_le(&payload[36], snap.position);
  payload[44] = static_cast<uint8_t>(safety_state);
  payload[45] = inputs;
  payload[46] = encoder_timer_ok ? 1 : 0;
  payload[47] = 0;
  payload[47] |= kTestMode ? (1u << 0) : 0;
  payload[47] |= can_backend_ok ? (1u << 1) : 0;
#if BOARD_ENABLE_BUILTIN_CAN_LANE
  payload[47] |= builtin_can_tx_ok ? (1u << 2) : 0;
#endif
#if BOARD_ENABLE_MCP2515
  payload[47] |= (BOARD_CAN_IRQ_MODE != 0) ? (1u << 3) : 0;
  payload[47] |= (BOARD_CAN_IRQ_MODE == 2) ? (1u << 4) : 0;
#endif
#if BOARD_ENABLE_VOLTAGE_ADC
  payload[47] |= voltage_adc_ok ? (1u << 5) : 0;
#endif
  payload[47] |= serial_tx_queued_bytes() > 0 ? (1u << 6) : 0;
  payload[47] |= serial_record_drop_total > 0 ? (1u << 7) : 0;
  wr_u32_le(&payload[48], snap.fault_flags);
  payload[52] = 2;  // BOARD_HEALTH payload version.
  payload[53] = static_cast<uint8_t>(sizeof(payload));
  payload[54] = static_cast<uint8_t>(safety_supervisor.state());
  payload[55] = safety_supervisor.faultBits();
  wr_u32_le(&payload[56], safety_supervisor.heartbeatAgeMs(millis()));
  wr_u32_le(&payload[60], safety_supervisor.leaseRemainingMs(millis()));
  wr_u32_le(&payload[64], host_frame_crc_failed_total);
  wr_u32_le(&payload[68], host_can_tx_request_total);
  wr_u32_le(&payload[72], host_can_tx_accepted_total);
  wr_u32_le(&payload[76], host_can_tx_rejected_total);
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  wr_u32_le(&payload[80], mcp2515_tx_total);
  wr_u32_le(&payload[84], mcp2515_tx_failed_total);
#endif
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  wr_u32_le(&payload[88], builtin_can_tx_total);
  wr_u32_le(&payload[92], builtin_can_tx_failed_total);
#endif
#if BOARD_ENABLE_MCP2515
  wr_u32_le(&payload[96], mcp_service.spi_error_total);
  wr_u32_le(&payload[100], mcp_service.error_flag_total);
  payload[104] = mcp_service.last_canintf;
  payload[105] = mcp_service.last_eflg;
  payload[106] = mcp_service.last_canctrl;
  payload[107] = mcp_service.last_int_low ? 1 : 0;
#endif
  wr_u32_le(&payload[108], queued);
  wr_u32_le(&payload[112], safety_supervisor.transitionCounter());
  uint32_t backend_flags = 0;
  backend_flags |= can_backend_ok ? (1u << 0) : 0;
#if BOARD_ENABLE_BUILTIN_CAN_LANE
  backend_flags |= builtin_can_tx_ok ? (1u << 1) : 0;
#endif
  wr_u32_le(&payload[116], backend_flags);
  wr_u32_le(&payload[120], host_heartbeat_total);
  wr_u32_le(&payload[124], host_control_session_total);
  emit_record(RecordType::BoardHealth, payload, sizeof(payload));
}

static bool init_voltage_adc_lane() {
#if BOARD_ENABLE_VOLTAGE_ADC
  analogReadResolution(kVoltageAdcBits);
  for (uint8_t i = 0; i < kVoltageChannelCount; ++i) {
    pinMode(kVoltagePins[i], INPUT);
    (void)analogRead(kVoltagePins[i]);
  }
  voltage_adc_sample_total = 0;
  voltage_adc_drop_total = 0;
  last_voltage_adc_sample_ms = millis();
  return true;
#else
  return false;
#endif
}

static void service_voltage_adc_lane() {
#if BOARD_ENABLE_VOLTAGE_ADC
  if (!voltage_adc_ok) {
    return;
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_voltage_adc_sample_ms < BOARD_VOLTAGE_SAMPLE_PERIOD_MS) {
    return;
  }
  last_voltage_adc_sample_ms = now_ms;

  uint8_t payload[44];
  memset(payload, 0, sizeof(payload));
  wr_u64_le(&payload[0], mono64_us());
  wr_u32_le(&payload[8], voltage_adc_sample_total);
  wr_u32_le(&payload[12], voltage_adc_drop_total);
  payload[16] = 0;                    // source 0: Portenta lab ADC
  payload[17] = kVoltageChannelCount;
  payload[18] = kVoltageAdcBits;
  payload[19] = 0x03;                 // bit0 raw-valid, bit1 direct MCU ADC

  for (uint8_t i = 0; i < kVoltageChannelCount; ++i) {
    payload[20 + i] = kVoltageChannelIds[i];
    int raw = analogRead(kVoltagePins[i]);
    if (raw < 0) {
      raw = 0;
      voltage_adc_drop_total++;
      payload[19] |= 0x80;
    }
    if (raw > ((1 << kVoltageAdcBits) - 1)) {
      raw = (1 << kVoltageAdcBits) - 1;
      payload[19] |= 0x40;
    }
    wr_u16_le(&payload[28 + (i * 2)], static_cast<uint16_t>(raw));
  }

  voltage_adc_sample_total++;
  emit_record(RecordType::AdcSample, payload, sizeof(payload));
#endif
}

static void init_safety_pins() {
#if BOARD_ENABLE_SAFETY_IO
  pinMode(BoardPins::CanTxEnable, OUTPUT);
  digitalWrite(BoardPins::CanTxEnable, LOW);

  pinMode(BoardPins::SafetyWatchdogToggle, OUTPUT);
  digitalWrite(BoardPins::SafetyWatchdogToggle, LOW);

  pinMode(BoardPins::EstopInN, INPUT_PULLUP);
  pinMode(BoardPins::ArmKeyIn, INPUT);
  pinMode(BoardPins::FieldPowerOk, INPUT_PULLUP);
  pinMode(BoardPins::EncoderFaultN, INPUT_PULLUP);
  pinMode(BoardPins::SpareServiceGpio, INPUT);
#endif
}

static bool should_enable_can_tx_gate_for_test() {
#if BOARD_ENABLE_CAN_TX_GATE_FOR_TEST && BOARD_ENABLE_BUILTIN_CAN_LANE
  return builtin_can_tx_ok;
#else
  return false;
#endif
}

static bool control_backend_ready_for_bus(uint8_t bus) {
#if BOARD_ENABLE_MCP2515 && BOARD_ENABLE_HOST_CAN_TX_MCP2515
  if (bus == BOARD_MCP2515_BUS_ID) {
    return can_backend_ok && mcp2515 != nullptr;
  }
#endif
#if BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  if (bus == BOARD_BUILTIN_CAN_BUS_ID) {
    return builtin_can_tx_ok;
  }
#endif
  return false;
}

static bool any_control_backend_ready() {
  bool ready = false;
#if BOARD_ENABLE_MCP2515 && BOARD_ENABLE_HOST_CAN_TX_MCP2515
  ready = ready || (can_backend_ok && mcp2515 != nullptr);
#endif
#if BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  ready = ready || builtin_can_tx_ok;
#endif
  return ready;
}

static csm::board::SafetyInputs read_safety_inputs() {
  csm::board::SafetyInputs inputs;
#if BOARD_ENABLE_SAFETY_IO
  inputs.estop_asserted = !digitalRead(BoardPins::EstopInN);
  inputs.field_power_ok = digitalRead(BoardPins::FieldPowerOk);
  inputs.encoder_fault = !digitalRead(BoardPins::EncoderFaultN);
  inputs.arm_key = digitalRead(BoardPins::ArmKeyIn);
#endif
  inputs.control_backend_ready = any_control_backend_ready();
  return inputs;
}

static void update_safety_state() {
#if BOARD_ENABLE_SAFETY_IO
  const csm::board::SafetyInputs inputs = read_safety_inputs();
  const SafetyState before = safety_supervisor.state();
  safety_supervisor.update(millis(), inputs);
  safety_state = safety_supervisor.state();
  digitalWrite(BoardPins::CanTxEnable,
               (safety_supervisor.canDriveTxGate() || should_enable_can_tx_gate_for_test()) ? HIGH : LOW);

  if (inputs.estop_asserted && !estop_prev) {
    emit_board_event(EventEstopAsserted, 0, 1);
  }
  if (!inputs.field_power_ok && field_power_prev) {
    emit_board_event(EventFieldPowerLost, 0, 1);
  }
  if (inputs.encoder_fault && !encoder_fault_prev) {
    encoder_fault_events++;
    emit_board_event(EventEncoderFaultAsserted, 0, encoder_fault_events);
  }
  if (before != safety_state) {
    emit_board_event(EventSafetyStateChanged,
                     (static_cast<uint16_t>(before) << 8) | static_cast<uint8_t>(safety_state),
                     safety_supervisor.transitionCounter());
  }

  estop_prev = inputs.estop_asserted;
  field_power_prev = inputs.field_power_ok;
  encoder_fault_prev = inputs.encoder_fault;
#else
  const csm::board::SafetyInputs inputs = read_safety_inputs();
  safety_supervisor.update(millis(), inputs);
  safety_state = safety_supervisor.state();
#endif
}

static void toggle_safety_watchdog_if_needed() {
#if BOARD_ENABLE_SAFETY_IO
  const uint32_t now_ms = millis();
  if (now_ms - last_watchdog_toggle_ms >= 50) {
    digitalWrite(BoardPins::SafetyWatchdogToggle, !digitalRead(BoardPins::SafetyWatchdogToggle));
    last_watchdog_toggle_ms = now_ms;
  }
#endif
}

#if BOARD_ENABLE_MCP2515
static constexpr uint8_t kMcpRegCanctrl = 0x0F;
static constexpr uint8_t kMcpRegCanintf = 0x2C;
static constexpr uint8_t kMcpRegEflg = 0x2D;
static constexpr uint8_t kMcpRegTxb0ctrl = 0x30;
static constexpr uint8_t kMcpInstrRead = 0x03;
static constexpr uint8_t kMcpInstrBitModify = 0x05;
static constexpr uint8_t kMcpCanctrlAbat = 0x10;
static constexpr uint8_t kMcpCanctrlOsm = 0x08;
static constexpr uint8_t kMcpTxbAbtf = 0x40;
static constexpr uint8_t kMcpTxbMloa = 0x20;
static constexpr uint8_t kMcpTxbTxerr = 0x10;
static constexpr uint8_t kMcpTxbTxreq = 0x08;

static uint8_t mcp2515_raw_read_register(uint8_t reg) {
  SPI.beginTransaction(SPISettings(kMcp2515SpiHz, MSBFIRST, SPI_MODE0));
  digitalWrite(BoardPins::CanSpiCsN, LOW);
  SPI.transfer(kMcpInstrRead);
  SPI.transfer(reg);
  const uint8_t value = SPI.transfer(0x00);
  digitalWrite(BoardPins::CanSpiCsN, HIGH);
  SPI.endTransaction();
  return value;
}

static void __attribute__((unused)) mcp2515_raw_modify_register(uint8_t reg, uint8_t mask, uint8_t data) {
  SPI.beginTransaction(SPISettings(kMcp2515SpiHz, MSBFIRST, SPI_MODE0));
  digitalWrite(BoardPins::CanSpiCsN, LOW);
  SPI.transfer(kMcpInstrBitModify);
  SPI.transfer(reg);
  SPI.transfer(mask);
  SPI.transfer(data);
  digitalWrite(BoardPins::CanSpiCsN, HIGH);
  SPI.endTransaction();
}
#endif

static void __attribute__((unused)) reset_mcp_service_state() {
#if BOARD_ENABLE_MCP2515
  mcp2515_irq_pending = false;
  mcp2515_irq_edges = 0;
  memset(&mcp_service, 0, sizeof(mcp_service));
  mcp_service.int_pin_enabled = BOARD_CAN_IRQ_MODE != 0;
  mcp_service.last_probe_us = micros();
  mcp_service.last_housekeeping_ms = millis();
  mcp_service.last_error_event_ms = millis() - BOARD_CAN_ERROR_EVENT_PERIOD_MS;
#endif
}

static void __attribute__((unused)) emit_mcp_status_event_throttled(uint8_t stage,
                                                                    uint8_t canintf,
                                                                    uint8_t eflg) {
#if BOARD_ENABLE_MCP2515
  const uint32_t now_ms = millis();
  if (now_ms - mcp_service.last_error_event_ms < BOARD_CAN_ERROR_EVENT_PERIOD_MS) {
    return;
  }
  mcp_service.last_error_event_ms = now_ms;
  const uint16_t detail = (static_cast<uint16_t>(stage) << 8) | canintf;
  const uint32_t packed = (static_cast<uint32_t>(eflg) << 24) |
                          (static_cast<uint32_t>(mcp_service.last_canctrl) << 16) |
                          (mcp_service.spi_error_total & 0xFFFFu);
  emit_board_event(EventMcp2515Error, detail, packed);
#else
  (void)stage;
  (void)canintf;
  (void)eflg;
#endif
}

static void __attribute__((unused)) service_mcp2515_status_after_drain(bool force_event) {
#if BOARD_ENABLE_MCP2515
  if (mcp2515 == nullptr) {
    return;
  }

  const uint8_t canintf = mcp2515->getInterrupts();
  const uint8_t eflg = mcp2515->getErrorFlags();
  const uint8_t canctrl = mcp2515->getControlRegister();
  mcp_service.last_canintf = canintf;
  mcp_service.last_eflg = eflg;
  mcp_service.last_canctrl = canctrl;

  bool handled_status = false;
  if (eflg & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR)) {
    can_fifo_overflow_total++;
    mcp_service.error_flag_total++;
    mcp2515->clearRXnOVRFlags();
    mcp2515->clearERRIF();
    handled_status = true;
  } else if (canintf & MCP2515::CANINTF_ERRIF) {
    mcp_service.error_flag_total++;
    mcp2515->clearERRIF();
    handled_status = true;
  }

  if (canintf & MCP2515::CANINTF_MERRF) {
    mcp_service.error_flag_total++;
    mcp2515->clearMERR();
    handled_status = true;
  }

  if (canintf & (MCP2515::CANINTF_TX0IF | MCP2515::CANINTF_TX1IF | MCP2515::CANINTF_TX2IF)) {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
    if (!mcp2515_pending_tx.active) {
      mcp2515->clearTXInterrupts();
    }
#else
    mcp2515->clearTXInterrupts();
#endif
    handled_status = true;
  }

  const bool rx_irq_left = (canintf & (MCP2515::CANINTF_RX0IF | MCP2515::CANINTF_RX1IF)) != 0;
  const bool int_low = (BOARD_CAN_IRQ_MODE != 0) && !digitalRead(BoardPins::CanIntN);
  mcp_service.last_int_low = int_low;
  mcp2515_irq_pending = int_low || rx_irq_left;

  if (force_event || handled_status) {
    emit_mcp_status_event_throttled(6, canintf, eflg);
  }
#else
  (void)force_event;
#endif
}

static bool __attribute__((unused)) should_service_mcp2515_rx() {
#if BOARD_ENABLE_MCP2515
  if (BOARD_CAN_IRQ_MODE == 0) {
    return true;
  }

  const uint32_t now_us = micros();
  const bool fallback_due =
      static_cast<uint32_t>(now_us - mcp_service.last_probe_us) >= BOARD_CAN_POLL_FALLBACK_US;
  const bool int_low = !digitalRead(BoardPins::CanIntN);
  if (int_low) {
    mcp_service.int_low_samples++;
  }

  bool edge_seen = false;
#if BOARD_CAN_IRQ_MODE == 2
  noInterrupts();
  const uint32_t edges = mcp2515_irq_edges;
  const bool pending = mcp2515_irq_pending;
  mcp2515_irq_pending = false;
  interrupts();
  edge_seen = pending || edges != mcp_service.last_seen_irq_edges;
  mcp_service.last_seen_irq_edges = edges;
#endif

  if (int_low || edge_seen || fallback_due) {
    mcp_service.last_probe_us = now_us;
    return true;
  }
  return false;
#else
  return false;
#endif
}

static bool init_can_backend() {
#if BOARD_ENABLE_MCP2515
  reset_mcp_service_state();
  pinMode(BoardPins::CanSpiCsN, OUTPUT);
  digitalWrite(BoardPins::CanSpiCsN, HIGH);
#if BOARD_CAN_IRQ_MODE != 0
  pinMode(BoardPins::CanIntN, INPUT_PULLUP);
#endif

  static MCP2515 can_controller(BoardPins::CanSpiCsN, kMcp2515SpiHz);
  mcp2515 = &can_controller;

  SPI.begin();

  auto emit_spi_snapshot = [&](uint8_t stage) {
    const uint8_t canstat = mcp2515_raw_read_register(0x0E);
    const uint8_t canctrl = mcp2515_raw_read_register(kMcpRegCanctrl);
    const uint8_t canintf = mcp2515_raw_read_register(kMcpRegCanintf);
    const uint8_t eflg = mcp2515_raw_read_register(kMcpRegEflg);
    const uint16_t detail = (static_cast<uint16_t>(stage) << 8) | canstat;
    const uint32_t packed = (static_cast<uint32_t>(canctrl) << 24) |
                            (static_cast<uint32_t>(canintf) << 16) |
                            (static_cast<uint32_t>(eflg) << 8);
    emit_board_event(EventMcp2515SpiSnapshot, detail, packed);
  };

  emit_spi_snapshot(1);
  mcp2515->reset();
  emit_spi_snapshot(2);
  mcp2515->clearTXInterrupts();
  mcp2515->clearERRIF();
  mcp2515->clearMERR();
  mcp2515->clearRXnOVRFlags();

  MCP2515::ERROR err = mcp2515->setBitrate(kMcp2515Bitrate, kMcp2515Clock);
  if (err != MCP2515::ERROR_OK) {
    emit_spi_snapshot(3);
    emit_board_event(EventMcp2515Error, static_cast<uint16_t>(err), 1);
    return false;
  }
  err = mcp2515->setNormalMode();
#if BOARD_MCP2515_TX_USE_ONESHOT
  if (err == MCP2515::ERROR_OK) {
    mcp2515_raw_modify_register(kMcpRegCanctrl, kMcpCanctrlOsm, kMcpCanctrlOsm);
  }
#endif
  if (err != MCP2515::ERROR_OK) {
    emit_spi_snapshot(4);
    emit_board_event(EventMcp2515Error, static_cast<uint16_t>(err), 2);
    return false;
  }
#if BOARD_CAN_IRQ_MODE == 2
  if (!mcp_service.exti_attached) {
    attachInterrupt(digitalPinToInterrupt(BoardPins::CanIntN), on_mcp2515_int, FALLING);
    mcp_service.exti_attached = true;
  }
#endif
#if BOARD_CAN_IRQ_MODE != 0
  mcp_service.last_int_low = !digitalRead(BoardPins::CanIntN);
  mcp2515_irq_pending = mcp_service.last_int_low;
#endif
  emit_spi_snapshot(5);
  return true;
#else
  return false;
#endif
}

static void pump_can_rx_to_queue(int budget) {
#if BOARD_ENABLE_MCP2515
  if (mcp2515 == nullptr) {
    return;
  }
  if (!should_service_mcp2515_rx()) {
    return;
  }

  bool saw_error = false;
  while (budget-- > 0) {
    struct can_frame msg;
    const MCP2515::ERROR err = mcp2515->readMessage(&msg);
    if (err == MCP2515::ERROR_NOMSG) {
      if ((BOARD_CAN_IRQ_MODE != 0) && !digitalRead(BoardPins::CanIntN)) {
        mcp_service.nomsg_while_int_low++;
      }
      break;
    }
    if (err != MCP2515::ERROR_OK) {
      mcp_service.spi_error_total++;
      saw_error = true;
      break;
    }

    CanRxItem item;
    item.mono_us = mono64_us();

    const bool is_ext = (msg.can_id & CAN_EFF_FLAG) != 0;
    const bool is_rtr = (msg.can_id & CAN_RTR_FLAG) != 0;
    const uint32_t id = is_ext ? (msg.can_id & CAN_EFF_MASK) : (msg.can_id & CAN_SFF_MASK);
    uint32_t can_id_flags = id & 0x1FFFFFFF;
    if (is_ext) {
      can_id_flags |= (1u << 29);
    }
    if (is_rtr) {
      can_id_flags |= (1u << 30);
    }
    item.can_id_flags = can_id_flags;

    uint8_t dlc = msg.can_dlc;
    if (dlc > 8) {
      dlc = 8;
    }
    item.dlc_flags = dlc & 0x0F;
    item.bus = BOARD_MCP2515_BUS_ID;
    memset(item.data, 0, sizeof(item.data));
    for (uint8_t i = 0; i < dlc; ++i) {
      item.data[i] = msg.data[i];
    }

    if (can_queue_push(item)) {
      can_rx_count_total++;
    }
  }
  service_mcp2515_status_after_drain(saw_error);
#else
  (void)budget;
#endif
}

static bool __attribute__((unused)) init_builtin_can_lane() {
#if BOARD_ENABLE_BUILTIN_CAN_LANE
  if (!CAN.begin(CanBitRate::BR_500k)) {
    emit_board_event(EventBuiltinCanBeginFailed, 0, 1);
    return false;
  }
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  builtin_can_tx_total = 0;
  builtin_can_tx_failed_total = 0;
#endif
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST
  last_builtin_can_tx_ms = millis() - BOARD_BUILTIN_CAN_TX_PERIOD_MS;
#endif
  return true;
#else
  return false;
#endif
}

static void service_builtin_can_rx_to_queue(int budget) {
#if BOARD_ENABLE_BUILTIN_CAN_RX
  if (!builtin_can_tx_ok) {
    return;
  }

  while (budget-- > 0 && CAN.available() > 0) {
    const CanMsg msg = CAN.read();

    CanRxItem item;
    item.mono_us = mono64_us();

    uint32_t can_id_flags = msg.isExtendedId() ? msg.getExtendedId() : msg.getStandardId();
    can_id_flags &= 0x1FFFFFFF;
    if (msg.isExtendedId()) {
      can_id_flags |= (1u << 29);
    }
    item.can_id_flags = can_id_flags;

    uint8_t dlc = msg.data_length;
    if (dlc > CanMsg::MAX_DATA_LENGTH) {
      dlc = CanMsg::MAX_DATA_LENGTH;
    }
    item.dlc_flags = dlc & 0x0F;
    item.bus = BOARD_BUILTIN_CAN_BUS_ID;
    memset(item.data, 0, sizeof(item.data));
    for (uint8_t i = 0; i < dlc; ++i) {
      item.data[i] = msg.data[i];
    }

    if (can_queue_push(item)) {
      can_rx_count_total++;
    }
  }
#else
  (void)budget;
#endif
}

static bool __attribute__((unused)) is_allowed_host_can_id(uint32_t can_id, bool extended) {
  if (extended) {
    return false;
  }
  return can_id == kHostCanTxAllowedPrimaryId ||
         (can_id >= kHostCanTxAllowedRangeStart && can_id <= kHostCanTxAllowedRangeEnd);
}

#if BOARD_ENABLE_MCP2515
static uint32_t __attribute__((unused)) mcp2515_tx_queue_count() {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  return (mcp2515_tx_q_head - mcp2515_tx_q_tail) & kMcp2515TxQueueMask;
#else
  return 0;
#endif
}

static bool __attribute__((unused)) enqueue_mcp2515_tx(uint8_t bus,
                                                       uint32_t can_id_flags,
                                                       uint8_t dlc,
                                                       const uint8_t* data) {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  const uint32_t next = (mcp2515_tx_q_head + 1) & kMcp2515TxQueueMask;
  if (next == mcp2515_tx_q_tail) {
    mcp2515_tx_queue_full_total++;
    return false;
  }

  Mcp2515PendingTx& item = mcp2515_tx_queue[mcp2515_tx_q_head];
  item.active = true;
  item.start_ms = 0;
  item.bus = bus;
  item.can_id_flags = can_id_flags;
  item.dlc = dlc;
  memset(item.data, 0, sizeof(item.data));
  memcpy(item.data, data, dlc > 8 ? 8 : dlc);
  mcp2515_tx_q_head = next;
  return true;
#else
  (void)bus;
  (void)can_id_flags;
  (void)dlc;
  (void)data;
  return false;
#endif
}

static MCP2515::ERROR __attribute__((unused)) start_mcp2515_tx_audit(uint8_t bus,
                                                                     uint32_t can_id_flags,
                                                                     uint8_t dlc,
                                                                     const uint8_t* data) {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  if (mcp2515 == nullptr || mcp2515_pending_tx.active) {
    return MCP2515::ERROR_ALLTXBUSY;
  }
  if ((mcp2515_raw_read_register(kMcpRegTxb0ctrl) & kMcpTxbTxreq) != 0) {
    return MCP2515::ERROR_ALLTXBUSY;
  }
  mcp2515_raw_modify_register(kMcpRegTxb0ctrl,
                              kMcpTxbAbtf | kMcpTxbMloa | kMcpTxbTxerr,
                              0);

  const bool extended = (can_id_flags & (1u << 29)) != 0;
  const uint32_t can_id = can_id_flags & 0x1FFFFFFF;

  struct can_frame msg;
  msg.can_id = can_id;
  if (extended) {
    msg.can_id |= CAN_EFF_FLAG;
  }
  msg.can_dlc = dlc;
  memset(msg.data, 0, sizeof(msg.data));
  memcpy(msg.data, data, dlc > 8 ? 8 : dlc);

  mcp2515->clearTXInterrupts();
  mcp2515->clearMERR();
  mcp2515->clearERRIF();

  const MCP2515::ERROR err = mcp2515->sendMessage(MCP2515::TXB0, &msg);
  if (err != MCP2515::ERROR_OK) {
    return err;
  }

  mcp2515_pending_tx.active = true;
  mcp2515_pending_tx.start_ms = millis();
  mcp2515_pending_tx.bus = bus;
  mcp2515_pending_tx.can_id_flags = can_id_flags;
  mcp2515_pending_tx.dlc = dlc;
  memset(mcp2515_pending_tx.data, 0, sizeof(mcp2515_pending_tx.data));
  memcpy(mcp2515_pending_tx.data, data, dlc > 8 ? 8 : dlc);
  return MCP2515::ERROR_OK;
#else
  (void)bus;
  (void)can_id_flags;
  (void)dlc;
  (void)data;
  return MCP2515::ERROR_FAILTX;
#endif
}

static bool __attribute__((unused)) start_next_mcp2515_queued_tx() {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  if (mcp2515_pending_tx.active || mcp2515 == nullptr || mcp2515_tx_q_tail == mcp2515_tx_q_head) {
    return false;
  }

  const Mcp2515PendingTx& item = mcp2515_tx_queue[mcp2515_tx_q_tail];
  const MCP2515::ERROR err = start_mcp2515_tx_audit(item.bus, item.can_id_flags, item.dlc, item.data);
  if (err == MCP2515::ERROR_OK) {
    mcp2515_tx_q_tail = (mcp2515_tx_q_tail + 1) & kMcp2515TxQueueMask;
    return true;
  }
  if (err == MCP2515::ERROR_ALLTXBUSY) {
    return false;
  }

  mcp2515_tx_q_tail = (mcp2515_tx_q_tail + 1) & kMcp2515TxQueueMask;
  mcp2515_tx_failed_total++;
  emit_board_event(EventMcp2515TxFailed, static_cast<uint16_t>(err), mcp2515_tx_failed_total);
  return false;
#else
  return false;
#endif
}

static void __attribute__((unused)) finish_mcp2515_pending_tx(bool success, uint16_t detail) {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  if (!mcp2515_pending_tx.active) {
    return;
  }

  if (success) {
    mcp2515_tx_total++;
    emit_can_tx_raw(mcp2515_pending_tx.bus, mcp2515_pending_tx.can_id_flags,
                    mcp2515_pending_tx.dlc, mcp2515_pending_tx.data,
                    mcp2515_tx_total, mcp2515_tx_failed_total);
  } else {
    mcp2515_tx_failed_total++;
    emit_board_event(EventMcp2515TxFailed, detail, mcp2515_tx_failed_total);
  }

  mcp2515_pending_tx.active = false;
  if (mcp2515 != nullptr) {
    mcp2515->clearTXInterrupts();
    mcp2515->clearMERR();
    mcp2515->clearERRIF();
    mcp2515_raw_modify_register(kMcpRegTxb0ctrl,
                                kMcpTxbAbtf | kMcpTxbMloa | kMcpTxbTxerr,
                                0);
  }
#else
  (void)success;
  (void)detail;
#endif
}

static void service_mcp2515_tx_audit() {
#if BOARD_ENABLE_MCP2515 && (BOARD_ENABLE_MCP2515_TX_TEST || BOARD_ENABLE_HOST_CAN_TX_MCP2515)
  if (!mcp2515_pending_tx.active || mcp2515 == nullptr) {
    start_next_mcp2515_queued_tx();
    return;
  }

  const uint8_t canintf = mcp2515_raw_read_register(kMcpRegCanintf);
  const uint8_t ctrl = mcp2515_raw_read_register(kMcpRegTxb0ctrl);
  const uint8_t eflg = mcp2515_raw_read_register(kMcpRegEflg);
  const bool tx0_done = (canintf & MCP2515::CANINTF_TX0IF) != 0;
  const bool txreq = (ctrl & kMcpTxbTxreq) != 0;
  const bool tx_ctrl_error = (ctrl & (kMcpTxbAbtf | kMcpTxbMloa | kMcpTxbTxerr)) != 0;
  const bool tx_bus_error = (eflg & (MCP2515::EFLG_TXBO | MCP2515::EFLG_TXEP)) != 0;

  if (tx_ctrl_error || tx_bus_error) {
    const uint16_t detail = (static_cast<uint16_t>(ctrl) << 8) | eflg;
    finish_mcp2515_pending_tx(false, detail);
    start_next_mcp2515_queued_tx();
    return;
  }

  if (tx0_done || !txreq) {
    finish_mcp2515_pending_tx(true, 0);
    start_next_mcp2515_queued_tx();
    return;
  }

  if (millis() - mcp2515_pending_tx.start_ms >= BOARD_MCP2515_TX_AUDIT_TIMEOUT_MS) {
    mcp2515_raw_modify_register(kMcpRegCanctrl, kMcpCanctrlAbat, kMcpCanctrlAbat);
    delayMicroseconds(20);
    mcp2515_raw_modify_register(kMcpRegCanctrl, kMcpCanctrlAbat, 0);
    const uint16_t detail = (static_cast<uint16_t>(ctrl) << 8) | eflg;
    finish_mcp2515_pending_tx(false, detail);
    start_next_mcp2515_queued_tx();
  }
#endif
}
#endif

static void handle_host_can_tx_request(const uint8_t* payload, uint16_t len) {
#if BOARD_ENABLE_HOST_CAN_TX_ANY
  uint32_t command_id = 0;
  uint8_t bus = 0;
  uint8_t dlc = 0;
  uint32_t can_id_flags = 0;

  host_can_tx_request_total++;

  if (len != 19) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadLength, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonBadLength, host_can_tx_rejected_total);
    return;
  }

  command_id = rd_u32_le(&payload[0]);
  bus = payload[4];
  const uint8_t frame_flags = payload[5];
  const bool extended = (frame_flags & 0x01) != 0;
  const bool rtr = (frame_flags & 0x02) != 0;
  const uint32_t raw_can_id = rd_u32_le(&payload[6]);
  dlc = payload[10];
  can_id_flags = raw_can_id & 0x1FFFFFFF;
  if (extended) {
    can_id_flags |= (1u << 29);
  }
  if (rtr) {
    can_id_flags |= (1u << 30);
  }

  const bool target_mcp2515 = (bus == BOARD_MCP2515_BUS_ID);
  const bool target_builtin_can = (bus == BOARD_BUILTIN_CAN_BUS_ID);
  const bool supported_bus =
      (target_mcp2515 && BOARD_ENABLE_HOST_CAN_TX_MCP2515 && BOARD_MCP2515_CONTROL_TX_ALLOWED) ||
      (target_builtin_can && BOARD_ENABLE_HOST_CAN_TX_BUILTIN && BOARD_BUILTIN_CAN_CONTROL_TX_ALLOWED);
  if (!supported_bus) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadBus, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonBadBus, host_can_tx_rejected_total);
    return;
  }
  if (rtr) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonUnsupportedFrame, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonUnsupportedFrame, host_can_tx_rejected_total);
    return;
  }
  if (dlc > 8) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonDlcOutOfRange, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonDlcOutOfRange, host_can_tx_rejected_total);
    return;
  }

  const uint32_t can_id = extended ? (raw_can_id & 0x1FFFFFFF) : (raw_can_id & 0x7FF);
  if (!is_allowed_host_can_id(can_id, extended)) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonIdNotAllowed, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonIdNotAllowed, host_can_tx_rejected_total);
    return;
  }

  uint8_t data[8] = {0};
  memcpy(data, &payload[11], dlc);

  uint8_t safety_reason = ControlReasonOk;
  if (!safety_supervisor.canAcceptTx(millis(), control_backend_ready_for_bus(bus), &safety_reason)) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, safety_reason, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, safety_reason, host_can_tx_rejected_total);
    return;
  }

#if BOARD_ENABLE_MCP2515 && BOARD_ENABLE_HOST_CAN_TX_MCP2515
  if (target_mcp2515) {
    if (!can_backend_ok || mcp2515 == nullptr) {
      host_can_tx_rejected_total++;
      emit_control_ack(command_id, ControlAckRejected, ControlReasonCanNotReady, bus, can_id_flags, dlc,
                       host_can_tx_request_total);
      emit_board_event(EventHostCanTxRejected, ControlReasonCanNotReady, host_can_tx_rejected_total);
      return;
    }

    if (!enqueue_mcp2515_tx(bus, can_id_flags, dlc, data)) {
      host_can_tx_rejected_total++;
      emit_control_ack(command_id, ControlAckRejected, ControlReasonQueueFull, bus, can_id_flags, dlc,
                       host_can_tx_request_total);
      emit_board_event(EventHostCanTxRejected, ControlReasonQueueFull, host_can_tx_rejected_total);
      return;
    }

    start_next_mcp2515_queued_tx();
    host_can_tx_accepted_total++;
    emit_control_ack(command_id, ControlAckAccepted, ControlReasonOk, bus, can_id_flags, dlc,
                     host_can_tx_accepted_total);
    safety_supervisor.noteControlTx(millis());
    return;
  }
#endif

#if BOARD_ENABLE_HOST_CAN_TX_BUILTIN
  if (!target_builtin_can) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadBus, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonBadBus, host_can_tx_rejected_total);
    return;
  }

  if (!builtin_can_tx_ok) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonCanNotReady, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonCanNotReady, host_can_tx_rejected_total);
    return;
  }

  const uint32_t arduino_id = extended ? arduino::CanExtendedId(can_id) : arduino::CanStandardId(can_id);
  CanMsg msg(arduino_id, dlc, data);
  const int rc = CAN.write(msg);
  if (rc <= 0) {
    builtin_can_tx_failed_total++;
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonCanWriteFailed, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventBuiltinCanTxFailed, static_cast<uint16_t>(-rc), builtin_can_tx_failed_total);
    return;
  }

  builtin_can_tx_total++;
  host_can_tx_accepted_total++;
  emit_control_ack(command_id, ControlAckAccepted, ControlReasonOk, bus, can_id_flags, dlc,
                   host_can_tx_accepted_total);
  safety_supervisor.noteControlTx(millis());
  emit_can_tx_raw(bus, can_id_flags, dlc, data, builtin_can_tx_total, builtin_can_tx_failed_total);
#else
  host_can_tx_rejected_total++;
  emit_control_ack(command_id, ControlAckRejected, ControlReasonBadBus, bus, can_id_flags, dlc,
                   host_can_tx_request_total);
  emit_board_event(EventHostCanTxRejected, ControlReasonBadBus, host_can_tx_rejected_total);
#endif
#else
  (void)payload;
  (void)len;
#endif
}

static void handle_host_heartbeat(uint16_t seq, const uint8_t* payload, uint16_t len) {
  uint32_t command_id = seq;
  if (len >= 4) {
    command_id = rd_u32_le(&payload[0]);
  }
  if (len != csm::kHostHeartbeatPayloadLen) {
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadLength, 0xFF, 0, 0,
                     host_heartbeat_total);
    return;
  }

  host_heartbeat_total++;
  safety_supervisor.heartbeat(millis());
  if ((host_heartbeat_total & 0x3Fu) == 1) {
    emit_board_event(EventHostHeartbeat, 0, host_heartbeat_total);
  }
}

static void handle_host_control_session(uint16_t seq, const uint8_t* payload, uint16_t len) {
  uint32_t command_id = seq;
  uint8_t action = 0xFF;
  uint8_t requested_bus = 0xFF;
  uint16_t lease_ms = 0;

  host_control_session_total++;

  if (len >= 4) {
    command_id = rd_u32_le(&payload[0]);
  }
  if (len != csm::kHostControlSessionPayloadLen) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadLength, requested_bus, 0, 0,
                     host_control_session_total);
    emit_board_event(EventHostControlSession, ControlReasonBadLength, host_control_session_total);
    return;
  }

  action = payload[4];
  requested_bus = payload[5];
  lease_ms = rd_u16_le(&payload[8]);

  const csm::board::SafetyInputs inputs = read_safety_inputs();
  safety_supervisor.update(millis(), inputs);
  safety_state = safety_supervisor.state();

  uint8_t reason = ControlReasonOk;
  uint8_t status = ControlAckAccepted;
  const bool backend_ready =
      requested_bus == 0xFF ? any_control_backend_ready() : control_backend_ready_for_bus(requested_bus);

  switch (action) {
    case csm::HostControlDisarm:
      safety_supervisor.disarm(millis());
      break;
    case csm::HostControlArm:
      reason = safety_supervisor.arm(millis(), lease_ms, backend_ready);
      break;
    case csm::HostControlRenewLease:
      reason = safety_supervisor.renewLease(millis(), lease_ms);
      break;
    case csm::HostControlInstallNeutralProfile:
      reason = ControlReasonUnsupportedCommand;
      break;
    default:
      reason = ControlReasonUnsupportedCommand;
      break;
  }

  if (reason != ControlReasonOk) {
    status = ControlAckRejected;
    host_can_tx_rejected_total++;
  }

  safety_state = safety_supervisor.state();
  emit_control_ack(command_id, status, reason, requested_bus, 0, 0, host_control_session_total);
  emit_board_event(EventHostControlSession,
                   (static_cast<uint16_t>(action) << 8) | reason,
                   host_control_session_total);
}

static void handle_host_query_capability(uint16_t seq, const uint8_t* payload, uint16_t len) {
  uint32_t command_id = seq;
  if (len >= 4) {
    command_id = rd_u32_le(&payload[0]);
  }
  if (len != 0 && len != 4) {
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadLength, 0xFF, 0, 0, 0);
    return;
  }
  emit_capability();
  emit_control_ack(command_id, ControlAckAccepted, ControlReasonOk, 0xFF, 0, 0, 0);
}

static void handle_host_clear_fault_lockout(uint16_t seq, const uint8_t* payload, uint16_t len) {
  uint32_t command_id = seq;
  if (len >= 4) {
    command_id = rd_u32_le(&payload[0]);
  }
  if (len != csm::kHostClearFaultLockoutPayloadLen) {
    emit_control_ack(command_id, ControlAckRejected, ControlReasonBadLength, 0xFF, 0, 0, 0);
    return;
  }

  const csm::board::SafetyInputs inputs = read_safety_inputs();
  uint8_t reason = ControlReasonOk;
  if (inputs.estop_asserted) {
    reason = ControlReasonEstopAsserted;
  } else if (!inputs.field_power_ok) {
    reason = ControlReasonFieldPowerLost;
  } else if (inputs.encoder_fault) {
    reason = csm::ControlReasonEncoderFault;
  }

  if (reason == ControlReasonOk) {
    safety_supervisor.clearFaultLockout(millis());
    safety_state = safety_supervisor.state();
    emit_control_ack(command_id, ControlAckAccepted, ControlReasonOk, 0xFF, 0, 0, 0);
    emit_board_event(EventFaultLockoutCleared, 0, safety_supervisor.transitionCounter());
  } else {
    emit_control_ack(command_id, ControlAckRejected, reason, 0xFF, 0, 0, 0);
  }
}

static void dispatch_host_frame(uint8_t version, uint8_t record_type, uint16_t seq,
                                const uint8_t* payload, uint16_t len) {
  if (version != kProtocolVersion) {
    emit_control_ack(seq, ControlAckRejected, ControlReasonBadProtocol, 0, 0, 0, host_can_tx_request_total);
    return;
  }

  if (record_type == static_cast<uint8_t>(RecordType::HostCanTxRequest)) {
    handle_host_can_tx_request(payload, len);
  } else if (record_type == static_cast<uint8_t>(RecordType::HostHeartbeat)) {
    handle_host_heartbeat(seq, payload, len);
  } else if (record_type == static_cast<uint8_t>(RecordType::HostControlSession)) {
    handle_host_control_session(seq, payload, len);
  } else if (record_type == static_cast<uint8_t>(RecordType::HostQueryCapability)) {
    handle_host_query_capability(seq, payload, len);
  } else if (record_type == static_cast<uint8_t>(RecordType::HostClearFaultLockout)) {
    handle_host_clear_fault_lockout(seq, payload, len);
  } else {
    emit_control_ack(seq, ControlAckRejected, ControlReasonUnsupportedCommand, 0xFF, 0, 0,
                     host_can_tx_request_total);
    emit_board_event(EventHostCommandUnsupported, record_type, host_can_tx_request_total);
  }
}

static void handle_host_downlink_frame(void*, uint8_t version, uint8_t record_type,
                                       uint16_t seq, const uint8_t* payload, uint16_t len) {
  dispatch_host_frame(version, record_type, seq, payload, len);
}

static void handle_host_downlink_crc_failure(void*) {
  host_frame_crc_failed_total++;
  if ((host_frame_crc_failed_total & 0x0F) == 1) {
    emit_board_event(EventHostFrameCrcFailed, 0, host_frame_crc_failed_total);
  }
}

static csm::board::HostDownlinkParser host_downlink_parser(
    handle_host_downlink_frame, handle_host_downlink_crc_failure);

static void service_host_downlink(int budget) {
  host_downlink_parser.service(Serial, budget);
}

static void service_builtin_can_tx_test() {
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST
  if (!builtin_can_tx_ok) {
    return;
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_builtin_can_tx_ms < BOARD_BUILTIN_CAN_TX_PERIOD_MS) {
    return;
  }
  last_builtin_can_tx_ms = now_ms;

  const uint32_t counter = builtin_can_tx_total;
  uint8_t data[8] = {
    0xB7,
    0x1D,
    static_cast<uint8_t>(counter & 0xFF),
    static_cast<uint8_t>((counter >> 8) & 0xFF),
    static_cast<uint8_t>((counter >> 16) & 0xFF),
    static_cast<uint8_t>((counter >> 24) & 0xFF),
    0x50,
    0x07,
  };

  CanMsg msg(arduino::CanStandardId(BOARD_BUILTIN_CAN_TX_TEST_ID), sizeof(data), data);
  const int rc = CAN.write(msg);
  if (rc <= 0) {
    builtin_can_tx_failed_total++;
    if ((builtin_can_tx_failed_total & 0x0F) == 1) {
      emit_board_event(EventBuiltinCanTxFailed, static_cast<uint16_t>(-rc), builtin_can_tx_failed_total);
    }
    return;
  }

  builtin_can_tx_total++;
  emit_can_tx_raw(BOARD_BUILTIN_CAN_BUS_ID, BOARD_BUILTIN_CAN_TX_TEST_ID, sizeof(data), data,
                  builtin_can_tx_total, builtin_can_tx_failed_total);
#endif
}

static void service_mcp2515_tx_test() {
#if BOARD_ENABLE_MCP2515 && BOARD_ENABLE_MCP2515_TX_TEST
  if (!can_backend_ok || mcp2515 == nullptr) {
    return;
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_mcp2515_tx_ms < BOARD_MCP2515_TX_PERIOD_MS) {
    return;
  }
  last_mcp2515_tx_ms = now_ms;

  const uint32_t counter = mcp2515_tx_total + mcp2515_tx_failed_total;
  uint8_t data[8] = {
    0xC5,
    0x10,
    static_cast<uint8_t>(counter & 0xFF),
    static_cast<uint8_t>((counter >> 8) & 0xFF),
    static_cast<uint8_t>((counter >> 16) & 0xFF),
    static_cast<uint8_t>((counter >> 24) & 0xFF),
    0x08,
    0x00,
  };

  const MCP2515::ERROR err = start_mcp2515_tx_audit(BOARD_MCP2515_BUS_ID, BOARD_MCP2515_TX_TEST_ID,
                                                    sizeof(data), data);
  if (err != MCP2515::ERROR_OK) {
    mcp2515_tx_failed_total++;
    if ((mcp2515_tx_failed_total & 0x0F) == 1) {
      emit_board_event(EventMcp2515TxFailed, static_cast<uint16_t>(err), mcp2515_tx_failed_total);
    }
    return;
  }
#endif
}

static void test_push_fake_can_if_needed() {
  static uint32_t last_fake_us = 0;
  const uint32_t now_us = micros();
  if (now_us - last_fake_us < 10000) {
    return;
  }
  last_fake_us = now_us;

  CanRxItem item;
  item.mono_us = mono64_us();
  item.can_id_flags = 0x123;
  item.dlc_flags = 8;
  item.bus = BOARD_MCP2515_BUS_ID;
  for (uint8_t i = 0; i < 8; ++i) {
    item.data[i] = i;
  }
  if (can_queue_push(item)) {
    can_rx_count_total++;
  }
}

static void drain_can_records(int budget) {
  CanRxItem item;
  while (budget-- > 0 && can_queue_pop(item)) {
    emit_can_rx_raw(item);
  }
}

static void service_encoder() {
  static int64_t last_position_for_velocity = 0;
  static uint32_t last_velocity_ms = 0;

  EncoderSnapshot snap = poll_encoder();

  if (encoder_index_pending) {
    noInterrupts();
    const uint64_t index_mono = encoder_index_mono_us;
    encoder_index_pending = false;
    interrupts();
    snap.mono_us = index_mono;
    emit_encoder_edge(snap, 0x01);
    emit_board_event(EventEncoderIndex, snap.ab_state, encoder_index_count);
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_encoder_derived_ms >= 50) {
    const uint32_t dt_ms = last_velocity_ms == 0 ? 50 : now_ms - last_velocity_ms;
    const int64_t delta = snap.position - last_position_for_velocity;
    int32_t cps = 0;
    if (dt_ms > 0) {
      cps = static_cast<int32_t>((delta * 1000) / static_cast<int64_t>(dt_ms));
    }
    emit_encoder_derived(snap, cps);
    last_position_for_velocity = snap.position;
    last_velocity_ms = now_ms;
    last_encoder_derived_ms = now_ms;
  }
}

void setup() {
  Serial.begin(115200);
  init_status_led();
  init_runtime_watchdog();
  const uint32_t start_ms = millis();
  while (!Serial && (millis() - start_ms) < 1500) {
    kick_runtime_watchdog();
  }

  init_safety_pins();
  safety_supervisor.begin(millis());
  safety_state = safety_supervisor.state();
  voltage_adc_ok = init_voltage_adc_lane();

#if BOARD_ENABLE_ENCODER_IO
  pinMode(BoardPins::EncoderA, INPUT);
  pinMode(BoardPins::EncoderB, INPUT);
  pinMode(BoardPins::EncoderZ, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BoardPins::EncoderZ), on_encoder_index, RISING);
#endif

  encoder_timer_ok = init_encoder_timer3();

  emit_capability();
  last_capability_ms = millis();
  emit_board_event(EventBoot, encoder_timer_ok ? 1 : 0, 1);
#if BOARD_HW_PROFILE_MID_TJA1051_DUAL && BOARD_TARGET_INTERNAL_CAN_LANE0 && !BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
  emit_board_event(EventCan0BackendUnavailable, 0, 1);
#endif

  if (!kTestMode && BOARD_ENABLE_MCP2515_INIT) {
    can_backend_ok = init_can_backend();
    if (!can_backend_ok) {
      emit_board_event(EventCanBeginFailed, 0, 1);
    }
  }

#if BOARD_ENABLE_BUILTIN_CAN_LANE
  builtin_can_tx_ok = init_builtin_can_lane();
#endif

  last_health_ms = millis();
  last_encoder_derived_ms = millis();
  last_watchdog_toggle_ms = millis();
}

void loop() {
  kick_runtime_watchdog();
  service_usb_cdc_reconnect_watchdog();
  mono64_us();
  service_serial_tx(512);
  service_host_downlink(256);
  update_safety_state();
  toggle_safety_watchdog_if_needed();

  if (kTestMode) {
    test_push_fake_can_if_needed();
  } else if (can_backend_ok) {
    pump_can_rx_to_queue(512);
  } else if (BOARD_ENABLE_MCP2515_INIT && (millis() - last_can_init_retry_ms >= 1000)) {
    last_can_init_retry_ms = millis();
    can_backend_ok = init_can_backend();
    if (!can_backend_ok) {
      emit_board_event(EventCanBeginFailed, 1, 1);
    }
  }

  service_builtin_can_rx_to_queue(128);
  service_builtin_can_tx_test();
#if BOARD_ENABLE_MCP2515
  service_mcp2515_tx_audit();
#endif
  service_mcp2515_tx_test();
  service_status_led();
  service_capability_advertisement();
  service_encoder();
  drain_can_records(BOARD_CAN_SERIAL_DRAIN_BUDGET);
  if (!kTestMode && can_backend_ok) {
    pump_can_rx_to_queue(512);
  }
  service_voltage_adc_lane();

  const uint32_t now_ms = millis();
  if (now_ms - last_health_ms >= 1000) {
    const EncoderSnapshot snap = poll_encoder();
    emit_board_health(snap);
    last_health_ms = now_ms;
  }
  service_serial_tx(1024);
  service_usb_cdc_reconnect_watchdog();
  kick_runtime_watchdog();
}
