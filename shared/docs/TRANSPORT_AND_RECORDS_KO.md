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

`CAN_RX_RAW` and `CAN_TX_RAW` payload, 30 bytes:
- `0..7 mono_us u64`
- `8..11 can_id_flags u32`: bit 0..28 ID, bit 29 extended, bit 30 RTR
- `12 dlc_flags u8`: low nibble DLC
- `13 bus u8`: logical bus id. Interpret the physical backend and role from
  `CAPABILITY`; do not hard-code MCP labels in new hosts.
- `14..21 data[8]`
- `22..25 total u32`: RX or TX success counter
- `26..29 dropped_or_failed u32`

Current board baseline:
- MCP2515 receive frames emit `CAN_RX_RAW bus=0`.
- Portenta built-in CAN receive frames emit `CAN_RX_RAW bus=1`.
- Successful Portenta built-in CAN writes emit `CAN_TX_RAW bus=1`.

Mid Carrier target baseline:
- `bus=0`: Mid Carrier J14 `CAN0_TX/RX` wired to ADA-5708 TJA1051T/3,
  monitor/system CAN role.
- `bus=1`: Mid Carrier J4 terminal CAN1 through onboard U2, drive/control CAN
  role.
- MCP2515 is not a production backend in this profile.

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
firmware emits `CONTROL_ACK status=1 reason=0` only after the immediate built-in
CAN write path accepts the request, then emits `CAN_TX_RAW` after the successful
write. Future queued control lanes may keep the same payload size while refining
status wording, but must preserve the rule that `CAN_TX_RAW` is the actual-send
evidence.

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

`HOST_CAN_TX_REQUEST` payload, 19 bytes, host-to-board:
- `0..3 command_id u32`
- `4 bus u8`: current control bus must be `1`
- `5 frame_flags u8`: bit0 extended, bit1 RTR
- `6..9 can_id u32`: raw CAN ID without flag bits
- `10 dlc u8`: `0..8`
- `11..18 data[8]`

Current board host TX policy:
- Accepted bus: `1` Portenta built-in CAN only.
- Accepted standard IDs: `0x503`, `0x510`, `0x511`, `0x512`, `0x513`.
- Extended and RTR frames are rejected in this baseline.
- On accepted hardware write, the board emits `CONTROL_ACK status=1 reason=0` and then `CAN_TX_RAW bus=1`.

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
  bit4 MCP INT_N level hint compiled, bit5 lab ADC evidence lane compiled
- `35 limitation/policy flags`: bit0 built-in CAN detailed bus-state counters are
  not exposed by the current API, bit1 host TX allowlist active, bit2 bench
  example TX active

Extended 80-byte `CAPABILITY` payload for board profile major `2`:
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
