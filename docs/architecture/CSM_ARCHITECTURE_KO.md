# CSM_ARCHITECTURE_KO

## 목표 구조
CSM firmware는 setup/loop orchestration과 lane/module 책임을 분리한다.

Target modules:
- `protocol/TypedFrame`: SOF, header, seq, length, CRC, endian helper
- `protocol/TypedRecords`: shared record id and payload helper
- `HostDownlinkParser`: USB downlink frame parser
- `ControlLane`: request validation, allowlist, TX queue, `CONTROL_ACK`
- `CanTxAuditLane`: actual successful write -> `CAN_TX_RAW`
- `CanLane_MCP2515`: current Mid Carrier MCP RX/TX -> `CAN_RX_RAW bus=0` and
  `CAN_TX_RAW bus=0`
- `CanRxLane_PortentaCan`: internal Classic CAN 2.0 lanes -> `CAN_RX_RAW bus=N`
- `AnalogSampleLane`: raw ADC -> `ADC_SAMPLE`
- `SafetySupervisor`: arm/lease/timeout/estop/neutral policy
- `HealthMonitor`: counters, queue depth, fault/event state
- `CapabilityPublisher`: lane and protocol capability evidence

## Current Migration Rule
Do behavior-preserving splits first for the legacy bench profile. The legacy
split must not change:
- record ids
- payload sizes
- endian layout
- MCP `bus=0`
- built-in CAN `bus=1`
- accepted host TX ids `0x503`, `0x510..0x513`

For the current Mid Carrier MCP2515 CSM target:
- `bus=0` means external MCP2515/TJA1050 over the Mid Carrier D7..D11 SPI pins.
- `bus=0` owns typed RX and allowlisted host-command TX audit.
- final dual CSM env `portenta_h7_m7_mid_mcp2515_j4_dual_csm` also exposes
  Mid Carrier J4 CAN1 through onboard U2 as `bus=1`; it owns typed RX and
  allowlisted host-command TX audit for the second physical CAN analyzer/bus.
- The internal CAN0/CAN1 + TJA1051 path is deferred.
- Qt/VMS must use `CAPABILITY` for labels, backend type, role, bitrate, and
  control permission.

## MCP2515 Rule
`INT_N` is active-low level information. It may be used as a hint with bounded
polling fallback, but the RX authority is MCP status/register drain.

## Production Cleanup
The built-in CAN and MCP example transmitters are bench features. Production
control builds must keep them behind build flags and rely on host requested
frames plus `CAN_TX_RAW` audit. The final runtime env disables periodic smoke
TX; the separate `portenta_h7_m7_mid_mcp2515_j4_dual_smoke` env is reserved for
hardware validation.

## VMS Freeze Boundary
Before VMS work, the CSM contract is frozen at typed transport v1:
- live stream records are `CAN_RX_RAW`, `CAN_TX_RAW`, `CONTROL_ACK`,
  `BOARD_EVENT`, `BOARD_HEALTH`, and `CAPABILITY` with the payload sizes in
  `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- bus meaning is descriptor-driven, not bus-number-driven
- Classic CAN 2.0 only, live DLC `0..8`; CAN FD is out of scope
- host control IDs are `0x503` and `0x510..0x513`
- `CONTROL_ACK` is not final TX evidence; VMS waits for matching `CAN_TX_RAW`
