# CSM_ARCHITECTURE_KO

## 목표 구조
CSM firmware는 setup/loop orchestration과 lane/module 책임을 분리한다.

Target modules:
- `protocol/TypedFrame`: SOF, header, seq, length, CRC, endian helper
- `protocol/TypedRecords`: shared record id and payload helper
- `HostDownlinkParser`: USB downlink frame parser
- `ControlLane`: request validation, allowlist, TX queue, `CONTROL_ACK`
- `CanTxAuditLane`: actual successful write -> `CAN_TX_RAW`
- `CanRxLane_LegacyMCP2515`: legacy bench MCP RX -> `CAN_RX_RAW bus=0`
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

For the Mid Carrier production target:
- `bus=0` means Mid Carrier J14 CAN0_TX/RX plus ADA-5708 TJA1051T/3.
- `bus=1` means Mid Carrier J4 terminal CAN1 through onboard U2.
- MCP2515 is not compiled into the production path.
- Qt/VMS must use `CAPABILITY` for labels, backend type, role, bitrate, and
  control permission.

## MCP2515 Rule
`INT_N` is active-low level information. It may be used as a hint with bounded
polling fallback, but the RX authority is MCP status/register drain.

## Production Cleanup
The built-in CAN `0x321` example transmitter is a bench feature. Production
control builds should gate it behind a build flag and rely on host requested
frames plus `CAN_TX_RAW` audit.
