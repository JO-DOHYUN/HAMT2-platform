#include <Arduino.h>
#include <cstring>

#include "BoardPins.h"
#include "protocol/TypedFrame.h"
#include "protocol/TypedRecords.h"

#ifndef BOARD_HW_PROFILE_MID_TJA1051_DUAL
#define BOARD_HW_PROFILE_MID_TJA1051_DUAL 0
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

#ifndef BOARD_CAN_ERROR_EVENT_PERIOD_MS
#define BOARD_CAN_ERROR_EVENT_PERIOD_MS 500
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

#define BOARD_ENABLE_BUILTIN_CAN_LANE (BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_BUILTIN_CAN_RX || BOARD_ENABLE_HOST_CAN_TX)

#ifndef BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
#define BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND 0
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
static constexpr uint32_t kMcp2515SpiHz = 250000;
#endif

using csm::crc16_ccitt;
using csm::kFrameSof0;
using csm::kFrameSof1;
using csm::kMaxPayloadLen;
using csm::kProtocolVersion;
using csm::kCapabilityV1PayloadLen;
using csm::kCapabilityV2BusDescriptorLen;
using csm::kCapabilityV2PayloadLen;
using csm::mono64_us;
using csm::rd_u16_le;
using csm::rd_u32_le;
using csm::RecordType;
using csm::wr_i32_le;
using csm::wr_i64_le;
using csm::wr_u16_le;
using csm::wr_u32_le;
using csm::wr_u64_le;

enum class SafetyState : uint8_t {
  MonitorOnly = 0,
  ControlStandby = 1,
  ControlArmed = 2,
  Fault = 3,
  Estop = 4,
};

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
static constexpr uint16_t kHostRxBufferSize = 160;
using csm::kControlBus;

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
static uint8_t host_rx_buf[kHostRxBufferSize];
static uint16_t host_rx_len = 0;
static uint32_t host_frame_crc_failed_total = 0;
static uint32_t host_can_tx_request_total = 0;
static uint32_t __attribute__((unused)) host_can_tx_accepted_total = 0;
static uint32_t host_can_tx_rejected_total = 0;

static volatile bool encoder_index_pending = false;
static volatile uint64_t encoder_index_mono_us = 0;
static volatile uint32_t encoder_index_count = 0;

static uint16_t transport_seq = 0;
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

#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX
static uint32_t builtin_can_tx_total = 0;
static uint32_t builtin_can_tx_failed_total = 0;
#endif

#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST
static uint32_t last_builtin_can_tx_ms = 0;
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
static SafetyState safety_state = SafetyState::MonitorOnly;

static uint32_t last_health_ms = 0;
static uint32_t last_encoder_derived_ms = 0;
static uint32_t last_watchdog_toggle_ms = 0;
static uint32_t last_can_init_retry_ms = 0;

static void emit_record(RecordType type, const uint8_t* payload, uint16_t len, uint8_t flags = 0) {
  if (csm::emit_typed_record(Serial, type, payload, len, transport_seq, flags)) {
    serial_record_tx_total++;
  }
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
using csm::ControlReasonDlcOutOfRange;
using csm::ControlReasonIdNotAllowed;
using csm::ControlReasonOk;
using csm::ControlReasonUnsupportedFrame;

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

static void emit_capability() {
  uint8_t payload[kCapabilityV2PayloadLen];
  memset(payload, 0, sizeof(payload));
  wr_u64_le(&payload[0], mono64_us());
  payload[8] = kProtocolVersion;
#if BOARD_HW_PROFILE_MID_TJA1051_DUAL
  payload[9] = 2;   // board profile major: Mid Carrier + TJA1051 target
#else
  payload[9] = 1;   // board profile major
#endif
  payload[10] = 0;  // board profile minor
  payload[11] = 1;  // mono64 unit: microsecond
  wr_u32_le(&payload[12], kCanQueueSize);
  wr_u32_le(&payload[16], 2048);   // DBS60E PPR
  wr_u32_le(&payload[20], 300000); // encoder channel frequency limit from datasheet
  payload[24] = 0x01;              // CAN_RX_RAW supported
  payload[25] = 0x01;              // CAN_TX_RAW supported
  payload[26] = 0x01;              // ENC_EDGE_RAW supported
  payload[27] = 0x01;              // ENC_DERIVED supported
  payload[28] = BOARD_ENABLE_VOLTAGE_ADC ? 0x01 : 0x00; // ADC_SAMPLE supported
  payload[29] = 0x01;              // BOARD_HEALTH supported
  payload[30] = 0x01;              // BOARD_EVENT supported
  payload[31] = BOARD_ENABLE_VOLTAGE_ADC ? kVoltageChannelCount : 0;
  payload[32] = BOARD_ENABLE_VOLTAGE_ADC ? kVoltageAdcBits : 0;
  payload[33] = static_cast<uint8_t>(BOARD_VOLTAGE_SAMPLE_PERIOD_MS & 0xFF);
  payload[34] = 0;                  // lane capability flags
#if BOARD_ENABLE_MCP2515
  payload[34] |= (1u << 0);         // MCP2515 bus=0 RX lane compiled
  payload[34] |= (BOARD_CAN_IRQ_MODE != 0) ? (1u << 4) : 0; // INT_N level hint
#endif
#if BOARD_ENABLE_BUILTIN_CAN_RX
  payload[34] |= (1u << 1);         // built-in CAN bus=1 RX lane compiled
#endif
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX
  payload[34] |= (1u << 2);         // built-in CAN bus=1 TX lane compiled
#endif
#if BOARD_ENABLE_HOST_CAN_TX
  payload[34] |= (1u << 3);         // host TX accepted on built-in CAN policy
#endif
#if BOARD_ENABLE_VOLTAGE_ADC
  payload[34] |= (1u << 5);         // lab ADC evidence lane compiled
#endif
  payload[35] = 0;                  // limitation/policy flags
#if BOARD_ENABLE_BUILTIN_CAN_LANE
  payload[35] |= (1u << 0);         // built-in CAN detailed bus-state counters not exposed
#endif
#if BOARD_ENABLE_HOST_CAN_TX
  payload[35] |= (1u << 1);         // host TX allowlist is active
#endif
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST
  payload[35] |= (1u << 2);         // bench example 0x321 transmitter is active
#endif

  uint16_t payload_len = kCapabilityV1PayloadLen;

#if BOARD_HW_PROFILE_MID_TJA1051_DUAL
  payload_len = kCapabilityV2PayloadLen;
  payload[36] = 2;                                // bus_count
  payload[37] = kCapabilityV2BusDescriptorLen;    // descriptor_size
  uint16_t cap_v2_flags = 0x0003;                 // descriptors present + production target
#if BOARD_TARGET_INTERNAL_CAN_LANE0 && !BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
  cap_v2_flags |= (1u << 2);                      // lane0 backend pending
#endif
  wr_u16_le(&payload[38], cap_v2_flags);

  auto write_bus_descriptor = [&](uint8_t offset, uint8_t bus_id, uint8_t role,
                                  uint8_t backend, uint8_t transceiver,
                                  uint8_t rx_supported, uint8_t tx_supported,
                                  uint8_t control_tx_allowed,
                                  uint8_t termination_policy,
                                  uint8_t isolation_policy) {
    payload[offset + 0] = bus_id;
    payload[offset + 1] = role;
    payload[offset + 2] = backend;
    payload[offset + 3] = transceiver;
    payload[offset + 4] = rx_supported;
    payload[offset + 5] = tx_supported;
    payload[offset + 6] = control_tx_allowed;
    payload[offset + 7] = 1;   // classic CAN supported
    payload[offset + 8] = 0;   // Classic CAN 2.0 only in this production profile
    payload[offset + 9] = 8;   // max live DLC in typed v1
    wr_u32_le(&payload[offset + 10], 500000);
    wr_u32_le(&payload[offset + 14], 0);
    payload[offset + 18] = termination_policy;
    payload[offset + 19] = isolation_policy;
  };

  write_bus_descriptor(
      40,
      0,
      1,  // monitor/system
#if BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
      3,  // STM32 HAL internal CAN
      2,  // ADA-5708 TJA1051T/3
      1,
      1,
#else
      4,  // unavailable/pending until internal CAN0 backend exists
      2,  // ADA-5708 TJA1051T/3 physical target
      0,
      0,
#endif
      0,
      2,  // selectable termination on ADA board
      1); // non-isolated bench breakout

  write_bus_descriptor(
      60,
      1,
      2,  // drive/control
      2,  // Arduino CAN single object on current Portenta variant
      3,  // Mid Carrier onboard U2 path
      BOARD_ENABLE_BUILTIN_CAN_RX ? 1 : 0,
      (BOARD_ENABLE_HOST_CAN_TX || BOARD_ENABLE_BUILTIN_CAN_TX_TEST) ? 1 : 0,
      BOARD_ENABLE_HOST_CAN_TX ? 1 : 0,
      0,  // termination policy must be verified on the actual carrier/bus
      0); // isolation policy unknown for the carrier path until schematic/HIL review
#endif

  emit_record(RecordType::Capability, payload, payload_len);
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

  uint8_t payload[52];
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
  wr_u32_le(&payload[48], snap.fault_flags);
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

static void update_safety_state() {
#if BOARD_ENABLE_SAFETY_IO
  const bool estop = !digitalRead(BoardPins::EstopInN);
  const bool field_ok = digitalRead(BoardPins::FieldPowerOk);
  const bool encoder_fault = !digitalRead(BoardPins::EncoderFaultN);

  if (estop) {
    safety_state = SafetyState::Estop;
    digitalWrite(BoardPins::CanTxEnable, LOW);
  } else if (!field_ok || encoder_fault) {
    safety_state = SafetyState::Fault;
    digitalWrite(BoardPins::CanTxEnable, LOW);
  } else {
    safety_state = SafetyState::MonitorOnly;
    digitalWrite(BoardPins::CanTxEnable, should_enable_can_tx_gate_for_test() ? HIGH : LOW);
  }

  if (estop && !estop_prev) {
    emit_board_event(EventEstopAsserted, 0, 1);
  }
  if (!field_ok && field_power_prev) {
    emit_board_event(EventFieldPowerLost, 0, 1);
  }
  if (encoder_fault && !encoder_fault_prev) {
    encoder_fault_events++;
    emit_board_event(EventEncoderFaultAsserted, 0, encoder_fault_events);
  }

  estop_prev = estop;
  field_power_prev = field_ok;
  encoder_fault_prev = encoder_fault;
#else
  safety_state = SafetyState::MonitorOnly;
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
    mcp2515->clearTXInterrupts();
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

  auto raw_read_register = [](uint8_t reg) -> uint8_t {
    SPI.beginTransaction(SPISettings(kMcp2515SpiHz, MSBFIRST, SPI_MODE0));
    digitalWrite(BoardPins::CanSpiCsN, LOW);
    SPI.transfer(0x03);  // MCP2515 READ
    SPI.transfer(reg);
    const uint8_t value = SPI.transfer(0x00);
    digitalWrite(BoardPins::CanSpiCsN, HIGH);
    SPI.endTransaction();
    return value;
  };

  auto emit_spi_snapshot = [&](uint8_t stage) {
    const uint8_t canstat = raw_read_register(0x0E);
    const uint8_t canctrl = raw_read_register(0x0F);
    const uint8_t canintf = raw_read_register(0x2C);
    const uint8_t eflg = raw_read_register(0x2D);
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
    item.bus = 0;
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
#if BOARD_ENABLE_BUILTIN_CAN_TX_TEST || BOARD_ENABLE_HOST_CAN_TX
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
    item.bus = 1;
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
  return can_id == 0x503 || (can_id >= 0x510 && can_id <= 0x513);
}

static void handle_host_can_tx_request(const uint8_t* payload, uint16_t len) {
#if BOARD_ENABLE_HOST_CAN_TX
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

  if (bus != kControlBus) {
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

  if (!builtin_can_tx_ok) {
    host_can_tx_rejected_total++;
    emit_control_ack(command_id, ControlAckRejected, ControlReasonCanNotReady, bus, can_id_flags, dlc,
                     host_can_tx_request_total);
    emit_board_event(EventHostCanTxRejected, ControlReasonCanNotReady, host_can_tx_rejected_total);
    return;
  }

  uint8_t data[8] = {0};
  memcpy(data, &payload[11], dlc);
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
  emit_can_tx_raw(bus, can_id_flags, dlc, data, builtin_can_tx_total, builtin_can_tx_failed_total);
#else
  (void)payload;
  (void)len;
#endif
}

static void dispatch_host_frame(uint8_t version, uint8_t record_type, uint16_t seq,
                                const uint8_t* payload, uint16_t len) {
  if (version != kProtocolVersion) {
    emit_control_ack(seq, ControlAckRejected, ControlReasonBadProtocol, 0, 0, 0, host_can_tx_request_total);
    return;
  }

  if (record_type == static_cast<uint8_t>(RecordType::HostCanTxRequest)) {
    handle_host_can_tx_request(payload, len);
  }
}

static void drop_host_rx_bytes(uint16_t count) {
  if (count >= host_rx_len) {
    host_rx_len = 0;
    return;
  }
  memmove(host_rx_buf, host_rx_buf + count, host_rx_len - count);
  host_rx_len -= count;
}

static void process_host_rx_buffer() {
  while (host_rx_len >= 2) {
    uint16_t start = 0;
    while (start + 1 < host_rx_len &&
           !(host_rx_buf[start] == kFrameSof0 && host_rx_buf[start + 1] == kFrameSof1)) {
      start++;
    }
    if (start > 0) {
      drop_host_rx_bytes(start);
    }
    if (host_rx_len < 9) {
      return;
    }
    if (host_rx_buf[0] != kFrameSof0 || host_rx_buf[1] != kFrameSof1) {
      drop_host_rx_bytes(1);
      continue;
    }

    const uint16_t payload_len = rd_u16_le(&host_rx_buf[7]);
    if (payload_len > kMaxPayloadLen) {
      drop_host_rx_bytes(1);
      continue;
    }

    const uint16_t frame_len = static_cast<uint16_t>(2 + 1 + 1 + 1 + 2 + 2 + payload_len + 2);
    if (host_rx_len < frame_len) {
      return;
    }

    const uint16_t expected_crc = rd_u16_le(&host_rx_buf[frame_len - 2]);
    const uint16_t actual_crc = crc16_ccitt(&host_rx_buf[2], static_cast<size_t>(frame_len - 4));
    if (expected_crc != actual_crc) {
      host_frame_crc_failed_total++;
      if ((host_frame_crc_failed_total & 0x0F) == 1) {
        emit_board_event(EventHostFrameCrcFailed, 0, host_frame_crc_failed_total);
      }
      drop_host_rx_bytes(1);
      continue;
    }

    dispatch_host_frame(host_rx_buf[2], host_rx_buf[3], rd_u16_le(&host_rx_buf[5]),
                        &host_rx_buf[9], payload_len);
    drop_host_rx_bytes(frame_len);
  }
}

static void service_host_downlink(int budget) {
  while (budget-- > 0 && Serial.available() > 0) {
    const int value = Serial.read();
    if (value < 0) {
      break;
    }
    if (host_rx_len >= kHostRxBufferSize) {
      drop_host_rx_bytes(1);
    }
    host_rx_buf[host_rx_len++] = static_cast<uint8_t>(value);
  }
  process_host_rx_buffer();
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

  CanMsg msg(arduino::CanStandardId(0x321), sizeof(data), data);
  const int rc = CAN.write(msg);
  if (rc <= 0) {
    builtin_can_tx_failed_total++;
    if ((builtin_can_tx_failed_total & 0x0F) == 1) {
      emit_board_event(EventBuiltinCanTxFailed, static_cast<uint16_t>(-rc), builtin_can_tx_failed_total);
    }
    return;
  }

  builtin_can_tx_total++;
  emit_can_tx_raw(1, 0x321, sizeof(data), data, builtin_can_tx_total, builtin_can_tx_failed_total);
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
  item.bus = 0;
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
  const uint32_t start_ms = millis();
  while (!Serial && (millis() - start_ms) < 1500) {}

  init_safety_pins();
  voltage_adc_ok = init_voltage_adc_lane();

#if BOARD_ENABLE_ENCODER_IO
  pinMode(BoardPins::EncoderA, INPUT);
  pinMode(BoardPins::EncoderB, INPUT);
  pinMode(BoardPins::EncoderZ, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BoardPins::EncoderZ), on_encoder_index, RISING);
#endif

  encoder_timer_ok = init_encoder_timer3();

  emit_capability();
  emit_board_event(EventBoot, encoder_timer_ok ? 1 : 0, 1);
#if BOARD_HW_PROFILE_MID_TJA1051_DUAL && BOARD_TARGET_INTERNAL_CAN_LANE0 && !BOARD_ENABLE_INTERNAL_CAN_LANE0_BACKEND
  emit_board_event(EventCan0BackendUnavailable, 0, 1);
#endif

  if (!kTestMode && BOARD_ENABLE_MCP2515_INIT) {
    can_backend_ok = init_can_backend();
    if (!can_backend_ok) {
      emit_board_event(EventCanBeginFailed, 0, 1);
      safety_state = SafetyState::Fault;
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
  mono64_us();
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
  service_encoder();
  drain_can_records(128);
  service_voltage_adc_lane();

  const uint32_t now_ms = millis();
  if (now_ms - last_health_ms >= 1000) {
    const EncoderSnapshot snap = poll_encoder();
    emit_board_health(snap);
    last_health_ms = now_ms;
  }
}
