# BRIEF.md - Repository Brief

## Current Focus
This repository is the CSM board-side reference package for the VMS-CSM typed
evidence/control architecture. The current task is to freeze the working CSM
baseline before VMS/Qt integration, not to reorganize the monorepo.

The current CSM baseline is:
- Arduino Portenta H7 M7 + Portenta Mid Carrier ASX00055, PlatformIO, Arduino framework.
- External MCP2515/TJA1050 module on Mid Carrier D7..D11 SPI pins, `MCP_8MHZ`,
  `CAN_500KBPS`, 2 MHz SPI in the current CSM profiles.
- MCP2515/TJA1050 typed receive lane as `CAN_RX_RAW bus=0`.
- MCP2515/TJA1050 host-command transmit lane as `CONTROL_ACK` then
  `CAN_TX_RAW bus=0`.
- Mid Carrier J4 CAN1 through onboard U2 as `CAN_RX_RAW bus=1` and audited
  host-command transmit as `CONTROL_ACK` then `CAN_TX_RAW bus=1` in
  `portenta_h7_m7_mid_mcp2515_j4_dual_csm`.
- USB typed binary stream v1 with CRC16 framing.
- Host downlink `HOST_CAN_TX_REQUEST` for audited bus0/bus1 control test frames
  on allowlisted standard IDs.
- Voltage raw lane as `ADC_SAMPLE`, not fake CAN.

Deferred hardware target:
- Dual internal CAN0/CAN1 through TJA1051-class transceivers is not the active
  implementation path in this repo. The current ArduinoCore Portenta target
  exposes only one practical CAN object, so the working CSM profile uses the
  external MCP2515 controller.

## Canonical Contracts
- Root routing: `AGENTS.md`
- CSM scoped rules: `board/AGENTS.md`, `board/BRIEF.md`
- VMS scoped rules: `qt/AGENTS.md`, `qt/BRIEF.md`
- Wire contract source of truth for both CSM and VMS:
  `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- Integration decision input: `VMS_CSM_03_ARCHITECT_SYNTHESIS_FINAL.md`

## Architecture Decision
CSM is not a transparent USB-CAN adapter. It publishes typed evidence and accepts
typed host control requests. The evidence chain is:

`host intent -> CONTROL_ACK accept/reject -> actual CAN write -> CAN_TX_RAW audit -> external feedback CAN_RX_RAW`

`CONTROL_ACK` alone is never final CAN success.

## Implementation Direction
- Keep `src/main.cpp` as setup/loop orchestration.
- Move transport/protocol primitives into protocol modules first.
- Split CAN, ADC, control, safety, health, and capability into lanes without
  changing wire format.
- Keep bus meaning, physical backend, role, bitrate, RX/TX support, and control
  policy capability-driven so Qt/VMS does not hard-code MCP labels.
- Keep legacy 20-byte files as replay/import compatibility only.

## Verification Baseline
Main build:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm
```

Hardware/HIL claims require an actual hardware run. A successful build alone is
not HIL evidence.

## VMS Next
VMS/Qt work starts from this CSM baseline. It should consume the shared typed
protocol, parse `CAPABILITY` before assigning bus labels, and keep
`CONTROL_ACK` separate from `CAN_TX_RAW` audit evidence.
