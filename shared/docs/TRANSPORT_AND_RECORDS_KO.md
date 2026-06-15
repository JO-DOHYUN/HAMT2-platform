# TRANSPORT_AND_RECORDS_KO

## 2026-04-22 canonical v1 addendum

This ASCII addendum is the active contract for the current board/Qt work. Older
Korean text below is retained as context, but implementations should follow this
section when there is a conflict.

Transport frame:
- SOF: `0xA5 0x5A`
- header: `version u8`, `record_type u8`, `flags u8`, `seq u16_le`, `payload_len u16_le`
- payload: record-specific little-endian binary
- trailer: `crc16_ccitt_le`
- CRC range: from `version` through final payload byte, excluding SOF and trailer
- receiver recovery: scan SOF, validate length, validate CRC, then dispatch

Record types:
- `1 CAN_RX_RAW`
- `2 CAN_TX_RAW`
- `3 ENC_EDGE_RAW`
- `4 ENC_DERIVED`
- `5 ADC_SAMPLE`
- `6 CONTROL_ACK`
- `7 BOARD_EVENT`
- `8 BOARD_HEALTH`
- `9 CAPABILITY`
- `10 HOST_CAN_TX_REQUEST` host-to-board downlink only
- `11 HOST_HEARTBEAT` host-to-board downlink only
- `12 HOST_CONTROL_SESSION` host-to-board downlink only
- `13 HOST_SET_CONTROL_POLICY` host-to-board downlink only, reserved
- `14 HOST_QUERY_CAPABILITY` host-to-board downlink only
- `15 HOST_CLEAR_FAULT_LOCKOUT` host-to-board downlink only
- `16 CAN_RX_SEGMENT`

Maximum payload length is `512` bytes for the current CSM rebuild. Hosts must
parse by `payload_len` and skip unknown trailing bytes.

`CAN_RX_RAW` and `CAN_TX_RAW` payload, 30 bytes:
- `0..7 mono_us u64`
- `8..11 can_id_flags u32`: bit 0..28 ID, bit 29 extended, bit 30 RTR
- `12 dlc_flags u8`: low nibble DLC
- `13 bus u8`: logical bus id. Interpret the physical backend and role from
  `CAPABILITY`; do not hard-code MCP labels in new hosts.
- `14..21 data[8]`
- `22..25 total u32`: RX or TX success counter
- `26..29 dropped_or_failed u32`

`CAN_RX_SEGMENT` payload, `32 + frame_count * 30` bytes:
- This is lossless packing for high-load receive. It is not compression,
  sampling, or summary data.
- Header:
  - `0..7 segment_seq64 u64`
  - `8..15 first_capture_seq64 u64`
  - `16..17 frame_count u16`
  - `18 entry_size u8`, currently `30`
  - `19 flags u8`, bit0 means `capture_seq64` is valid
  - `20..23 dropped_before_segment u32`
  - `24..27 fifo_before_segment u32`
  - `28..31 reserved u32`
- Entry, repeated `frame_count` times:
  - `0..7 capture_seq64 u64`
  - `8..15 mono_us u64`
  - `16..19 can_id_flags u32`
  - `20 dlc_flags u8`
  - `21 bus u8`
  - `22..29 data[8]`

Current board baseline:
- MCP2515 receive frames are emitted as `CAN_RX_SEGMENT` entries with `bus=0`.
- Portenta built-in CAN receive frames are emitted as `CAN_RX_SEGMENT` entries
  with `bus=1`.
- `CAN_RX_RAW` remains protocol-compatible for older hosts/builds, but the
  high-load dual-channel build uses `CAN_RX_SEGMENT` for live RX uplink.
- Successful Portenta built-in CAN writes emit `CAN_TX_RAW bus=1`.

Current Mid Carrier MCP2515 CSM profile:
- Profile major `3`.
- `bus=0`: external MCP2515/TJA1050 on Mid Carrier `D7..D11` SPI pins,
  Classic CAN 2.0, `MCP_8MHZ`, `CAN_500KBPS`. The current high-load dual CSM
  build profile uses polling drain (`BOARD_CAN_IRQ_MODE=0`), 8 MHz MCP SPI, and
  `BOARD_CAN_SERIAL_DRAIN_BUDGET=512`.
- `bus=0` emits `CAN_RX_SEGMENT` entries for received frames and `CAN_TX_RAW`
  for successful host-commanded or bench-test writes.
- final dual-channel runtime env `portenta_h7_m7_mid_mcp2515_j4_dual_csm`
  additionally exposes `bus=1`: Mid Carrier J4 CAN1 through onboard U2,
  Classic CAN 2.0, 500 kbps. `bus=1` emits `CAN_RX_SEGMENT` entries and accepts
  audited allowlisted host-commanded writes.
- The previous dual internal CAN0/CAN1 + TJA1051 target is deferred in this
  repository until a second internal CAN controller backend is implemented and
  HIL-proven.

`CONTROL_ACK` payload, 28 bytes:
- `0..7 mono_us u64`
- `8..11 command_id u32`
- `12 status u8`: `0` reject, `1` accepted by the board for processing in this v1 baseline
- `13 reason u8`: `0` ok, nonzero reject/failure reason
- `14 bus u8`
- `15 dlc_flags u8`
- `16..19 can_id_flags u32`
- `20..23 counter u32`: accepted counter or request counter depending on status
- `24..27 rejected_total u32`

`CONTROL_ACK` is request-decision evidence, not final CAN success evidence. Host
software must not mark a frame as actually sent from `CONTROL_ACK` alone. Actual
CAN TX success is proven by the matching `CAN_TX_RAW` audit record. The current
MCP2515 profile emits `CONTROL_ACK status=1 reason=0` after the request is
accepted into the MCP TX path, then emits `CAN_TX_RAW` only after the TX
completion audit succeeds. The built-in CAN profile emits `CAN_TX_RAW` after the
Arduino CAN API accepts the write. Future queued control lanes may keep the same
payload size while refining status wording, but must preserve the rule that
`CAN_TX_RAW` is the actual-send evidence.

Current `CONTROL_ACK` reasons:
- `0` ok
- `1` bad payload length
- `2` bad bus
- `3` unsupported frame flag such as RTR
- `4` DLC out of range
- `5` CAN ID not allowed
- `6` built-in CAN not ready
- `7` CAN write failed
- `8` bad protocol version
- `9` safety not armed
- `10` host heartbeat timeout
- `11` control lease expired
- `12` safety lockout
- `13` estop asserted
- `14` field power lost
- `15` encoder fault
- `16` queue full
- `17` TX busy
- `18` bus off
- `19` error passive
- `20` role unresolved
- `21` policy hash mismatch
- `22` neutral profile missing
- `23` rate limited
- `24` unsupported command

`HOST_CAN_TX_REQUEST` payload, 19 bytes, host-to-board:
- `0..3 command_id u32`
- `4 bus u8`: logical bus id from `CAPABILITY`; do not assume a fixed system
  or drive channel number
- `5 frame_flags u8`: bit0 extended, bit1 RTR
- `6..9 can_id u32`: raw CAN ID without flag bits
- `10 dlc u8`: `0..8`
- `11..18 data[8]`

Current board host TX policy:
- Accepted bus is profile-dependent. Profile major `1` accepts `bus=1` Portenta
  built-in CAN. Profile major `3` accepts the buses whose `CAPABILITY`
  descriptor has `control_tx_allowed=1`; the final dual CSM env accepts `bus=0`
  MCP2515/TJA1050 and `bus=1` Mid Carrier J4/U2.
- Accepted standard IDs: `0x503`, `0x510`, `0x511`, `0x512`, `0x513`.
- Extended and RTR frames are rejected in this baseline.
- On accepted hardware write, the board emits `CONTROL_ACK status=1 reason=0`
  and then `CAN_TX_RAW` on the same bus.

Safety-gated control session:
- `HOST_HEARTBEAT` payload, 12 bytes:
  - `0..3 command_id u32`
  - `4..7 host_mono_ms u32`
  - `8..9 flags u16`
  - `10..11 reserved u16`
- `HOST_CONTROL_SESSION` payload, 24 bytes:
  - `0..3 command_id u32`
  - `4 action u8`: `0` disarm, `1` arm, `2` renew lease, `3` install neutral profile reserved
  - `5 requested_bus u8`: physical bus id or `0xFF` for any configured control backend
  - `6..7 flags u16`
  - `8..9 lease_ms u16`: `0` means board default 500 ms, max 2000 ms
  - `10..11 reserved u16`
  - `12..15 policy_hash u32`
  - `16..19 model_pack_hash u32`
  - `20..23 aux u32`
- `HOST_QUERY_CAPABILITY` payload is either 0 bytes or `command_id u32`.
- `HOST_CLEAR_FAULT_LOCKOUT` payload is `command_id u32`.
- Production host TX requires heartbeat alive, arm accepted, lease valid, safe
  inputs, and a ready target backend. Heartbeat resume alone never auto-arms.

Reserved next-phase `CONTROL_ACK` status names, without changing the current v1
payload:
- `0 REJECTED`
- `1 ACCEPTED`
- `2 ACCEPTED_WRITTEN` optional if a future queued lane wants separate written acknowledgment
- `3 ACCEPTED_RATE_LIMITED` optional

Reserved next-phase `CONTROL_ACK` reasons:
- `9` safety not armed
- `10` host timeout
- `11` rate limited
- `12` queue full
- `13` estop active
- `14` fault active

Current `BOARD_EVENT` codes used by the reference firmware:
- `1` boot
- `2` legacy MCP/CAN backend begin failed
- `3` CAN RX queue drop
- `4` encoder fault asserted
- `5` field power lost
- `6` estop asserted
- `7` encoder index
- `8` encoder wrap
- `9` MCP2515 error
- `10` MCP2515 SPI snapshot
- `11` built-in CAN begin failed
- `12` built-in CAN TX failed
- `13` host frame CRC failed
- `14` host CAN TX rejected
- `15` host CAN TX accepted
- `16` Mid Carrier target CAN0 backend unavailable or pending
- `17` MCP2515 TX failed
- `18` safety state changed
- `19` host heartbeat observed
- `20` host control session decision
- `21` unsupported host command
- `22` fault lockout cleared
- `23` firmware identity boot marker. `detail low byte` is identity payload
  version, `detail high byte` is dirty flag, `counter` is firmware build id.
- `24` serial TX backpressure observed
- `25` serial TX ring clear. `detail` is clear reason and `counter` is dropped
  queued byte count.

`ADC_SAMPLE` payload, 44 bytes:
- `0..7 mono_us u64`
- `8..11 sample_total u32`
- `12..15 dropped_total u32`
- `16 source_id u8`: `0` Portenta lab ADC, later values may describe external ADC front-ends
- `17 channel_count u8`: maximum 8
- `18 resolution_bits u8`
- `19 flags u8`: bit0 raw valid, bit1 direct MCU ADC, bit6 saturated, bit7 read error
- `20..27 channel_id[8]`
- `28..43 raw_u16[8]`

Current voltage raw channel assignment:
- `channel_id 0`: Portenta `A0`
- `channel_id 1`: Portenta `A1`
- `channel_id 2`: Portenta `A6`
- `channel_id 3`: Portenta `A7`
- `A2/A3/A4/A5` are not used in the MCP bench because they overlap the `PC2/PC3` SPI pin family.

Qt storage rule:
- Store the original typed byte stream append-only. CAN, voltage, encoder, health, and events remain in one ordered binary stream.
- Indexes, decoded values, graphs, and operator notes are derived sidecars and must not replace the original stream.
- Board direct voltage samples must not be encoded as fake CAN frames.

`CAPABILITY` minimum meaning:
- A valid typed board should emit `CAPABILITY` after boot and after host reconnect
  if the implementation supports reconnect handling.
- Host software must use `CAPABILITY` to distinguish physical serial open from a
  typed board with matching protocol.
- CSM must not hide lane differences. The profile must expose bus role,
  physical backend, transceiver, RX/TX support, host TX policy, ADC lane
  presence, safety/control support level, and known API limitations such as
  unavailable built-in CAN FIFO/bus-off counters.
- When payload extensions are needed, keep record type `9` and version/profile
  fields explicit so older hosts can reject or degrade cleanly.
- Bus descriptor byte `1` is now `role_hint` only. The value is not
  authoritative and may be `0`. VSM must bind System/Drive semantics from its
  model pack, fingerprint, or operator override, not from the physical bus id.

Current 36-byte `CAPABILITY` payload interpretation:
- `0..7 mono_us u64`
- `8 protocol_version u8`
- `9 board_profile_major u8`
- `10 board_profile_minor u8`
- `11 mono64_unit u8`: `1` microsecond
- `12..15 can_queue_size u32`
- `16..19 encoder_ppr u32`
- `20..23 encoder_channel_frequency_limit_hz u32`
- `24 CAN_RX_RAW supported u8`
- `25 CAN_TX_RAW supported u8`
- `26 ENC_EDGE_RAW supported u8`
- `27 ENC_DERIVED supported u8`
- `28 ADC_SAMPLE supported u8`
- `29 BOARD_HEALTH supported u8`
- `30 BOARD_EVENT supported u8`
- `31 ADC channel count u8`
- `32 ADC resolution bits u8`
- `33 ADC sample period ms low byte u8`
- `34 lane capability flags`: bit0 MCP bus0 RX compiled, bit1 built-in bus1 RX
  compiled, bit2 built-in bus1 TX compiled, bit3 host TX policy active on bus1,
  bit4 MCP INT_N level hint compiled, bit5 lab ADC evidence lane compiled, bit6
  MCP bus0 TX compiled, bit7 host TX policy active on bus0
- `35 limitation/policy flags`: bit0 built-in CAN detailed bus-state counters are
  not exposed by the current API, bit1 host TX allowlist active, bit2 bench
  example TX active

Extended 80-byte `CAPABILITY` payload for board profile major `2` or `3`:
- `0..35`: same prefix as the current 36-byte payload.
- `36 bus_count u8`: current Mid Carrier profile uses `2`.
- `37 descriptor_size u8`: currently `20`.
- `38..39 capability_v2_flags u16_le`: bit0 bus descriptors present, bit1
  production target, bit2 lane0 backend pending.
- `40..59 bus descriptor 0`
- `60..79 bus descriptor 1`

Each 20-byte bus descriptor:
- `0 bus_id u8`
- `1 role u8`: `1` monitor/system, `2` drive/control, `3` debug/legacy
- `2 physical_backend u8`: `1` MCP2515 legacy, `2` Arduino CAN single object,
  `3` STM32 HAL internal CAN, `4` unavailable/pending
- `3 transceiver u8`: `1` TJA1050 legacy, `2` ADA-5708 TJA1051T/3, `3` Mid
  Carrier onboard U2, `255` unknown
- `4 rx_supported u8`
- `5 tx_supported u8`
- `6 control_tx_allowed u8`
- `7 classic_can_supported u8`
- `8 can_fd_supported u8`: hardware capability only; live v1 still sends
  classic CAN payloads with `data[8]`
- `9 max_dlc u8`: currently `8` for live v1
- `10..13 nominal_bitrate u32`: current target `500000`
- `14..17 data_bitrate u32`: `0` for the Classic CAN 2.0 profile
- `18 termination_policy u8`: `0` unknown, `1` off/default, `2` selectable,
  `3` on for bench endpoint
- `19 isolation_policy u8`: `0` unknown, `1` non-isolated bench, `2` carrier or
  external isolation required, `3` isolated production path

Mid Carrier profile major `2` descriptor defaults before the dual internal CAN
backend is
implemented:
- bus0 descriptor may advertise physical target and role while setting
  `rx_supported=0`, `tx_supported=0`, and physical_backend `4` if the firmware
  cannot yet open CAN0. This is not a supported live lane until HIL proves it.
- bus1 descriptor may advertise Arduino CAN single-object backend for the
  current Portenta `PH13/PB8` path through Mid Carrier J4.
- Hosts must tolerate longer `CAPABILITY` payloads by parsing the common prefix
  and skipping unknown trailing fields.

Extended 112-byte `CAPABILITY` payload:
- `0..79`: same as the 80-byte payload.
- `80..83 supported_uplink_records u32`: bit index equals record type value.
- `84..87 supported_downlink_records u32`: bit index equals record type value.
- `88..91 safety_feature_flags u32`
- `92..95 policy_hash u32`
- `96..99 firmware_build_id u32`
- `100..101 host_tx_queue_size u16`
- `102..103 capability_v3_flags u16`
- `104..111 reserved`

Extended 192-byte `CAPABILITY` payload:
- `0..111`: same as the 112-byte payload.
- `112 firmware_identity_version u8`: currently `1`
- `113 git_dirty u8`: `1` means the firmware was built from a dirty local tree
- `114 BOARD_CAN_IRQ_MODE u8`
- `115 reserved u8`
- `116..119 build_epoch_s u32`
- `120..123 firmware_build_id u32`
- `124..127 MCP SPI Hz u32`
- `128..129 CAN record drain budget u16`
- `130..131 serial TX ring size KiB u16`
- `132..143 git short sha ASCII, zero-padded`
- `144..191 PlatformIO env name ASCII, zero-padded`

Extended 128-byte `BOARD_HEALTH` payload:
- `0..51`: same prefix as the original 52-byte health payload.
- `52 health_payload_version u8`: `2` for the old extended payload, `4` for the
  high-load CSM payload
- `53 health_payload_len u8`
- `54 safety_state u8`
- `55 safety_fault_bits u8`
- `56..59 heartbeat_age_ms u32`
- `60..63 lease_remaining_ms u32`
- `64..67 host_crc_fail_total u32`
- `68..71 host_can_tx_request_total u32`
- `72..75 host_can_tx_accepted_total u32`
- `76..79 host_can_tx_rejected_total u32`
- `80..87 MCP TX success/fail counters`
- `88..95 J4 built-in CAN TX success/fail counters`
- `96..107 MCP SPI/error/register snapshot`
- `108..111 CAN RX queue depth u32`
- `112..115 safety_transition_counter u32`
- `116..119 backend_flags u32`
- `120..123 host_heartbeat_total u32`
- `124..127 host_control_session_total u32`

Extended 192-byte `BOARD_HEALTH v4` payload:
- `0..127`: same as the 128-byte payload.
- `128..131 bus0_rx_total u32`
- `132..135 bus0_drop_total u32`
- `136..139 bus0_queue_depth u32`
- `140..143 bus0_queue_high_water u32`
- `144..147 bus1_rx_total u32`
- `148..151 bus1_drop_total u32`
- `152..155 bus1_queue_depth u32`
- `156..159 bus1_queue_high_water u32`
- `160..163 serial_enqueue_fail_total u32`
- `164..167 serial_ring_clear_total u32`
- `168..171 serial_ring_cleared_bytes_total u32`
- `172..175 serial_backpressure_total u32`
- `176..179 serial_tx_high_water_bytes u32`
- `180..183 shared_can_queue_high_water u32`
- `184..187 MCP drain time-budget-hit total u32`
- `188..191 reserved`

Mid Carrier MCP2515 profile major `3` descriptor default:
- `bus_count` is build-profile driven. Single-lane MCP profiles use
  `bus_count=1`. The final dual CSM profile uses `bus_count=2`.
- descriptor 0 describes `bus=0` MCP2515/TJA1050.
- descriptor 1, when present, describes `bus=1` Mid Carrier J4/U2 through the
  Arduino CAN single-object backend.
- role is build-profile driven. The default `portenta_h7_m7_mid_mcp2515_csm`
  uses role `2` drive/control, while
  `portenta_h7_m7_mid_mcp2515_csm_system` uses role `1` monitor/system on the
  same physical MCP2515 channel. Qt/VMS must bind labels and control affordances
  from this descriptor, not from the bus number.
- physical backend `1` MCP2515, transceiver `1` TJA1050.
- `rx_supported=1`, `tx_supported=1`, `control_tx_allowed=1` when
  `portenta_h7_m7_mid_mcp2515_csm` is built.
- Classic CAN supported, CAN FD unsupported, max live DLC `8`, nominal bitrate
  `500000`, data bitrate `0`.

Final CSM protocol freeze for VMS:
- VMS must use transport `version=1` and the record IDs/payload sizes in this
  document without alternate live 20-byte modes.
- VMS must parse `CAPABILITY` first and bind bus labels, backend, role, bitrate,
  and control permission from descriptors.
- VMS may send `HOST_CAN_TX_REQUEST` only for allowlisted standard IDs
  `0x503` and `0x510..0x513`, DLC `0..8`, no RTR, no extended frame.
- VMS must treat `CONTROL_ACK` as board decision evidence only. Actual sent
  evidence is the matching `CAN_TX_RAW` on the requested bus.

## 권장 방향
- 링크 계층: framed transport
- 데이터 계층: typed records
- legacy 20B telemetry는 호환 옵션으로 유지

## framed transport v1
- SOF: `0xA5 0x5A`
- header:
  - `version`: `uint8`, 현재 `1`
  - `record_type`: `uint8`
  - `flags`: `uint8`
  - `seq`: `uint16_le`
  - `payload_len`: `uint16_le`
- payload: record type별 little-endian binary payload
- trailer: `crc16_ccitt_le`
- CRC 범위: `version`부터 payload 마지막 byte까지, SOF 제외
- 수신기는 SOF search + length + CRC로 resync한다.

## 최소 record type
- `1 CAN_RX_RAW`
- `2 CAN_TX_RAW`
- `3 ENC_EDGE_RAW`
- `4 ENC_DERIVED`
- `5 ADC_SAMPLE`
- `6 CONTROL_ACK`
- `7 BOARD_EVENT`
- `8 BOARD_HEALTH`
- `9 CAPABILITY`

## 금지
- board direct sensor 값을 가짜 CAN frame으로 위장
- raw를 derived로 덮어쓰기
- overflow/drop을 stats 뒤에 숨기기
