# BRIEF.md - Repository Brief

## Current Focus
This repository is the CSM board-side reference package for the VMS-CSM typed
evidence/control architecture.

The current CSM baseline is:
- Arduino Portenta H7 M7, PlatformIO, Arduino framework.
- MCP2515/TJA1050 receive lane as `CAN_RX_RAW bus=0`.
- Portenta built-in CAN receive and transmit audit lane as `bus=1`.
- USB typed binary stream v1 with CRC16 framing.
- Host downlink `HOST_CAN_TX_REQUEST` for built-in CAN control test frames.
- Voltage raw lane as `ADC_SAMPLE`, not fake CAN.

Next hardware target:
- Portenta H7 M7 + Portenta Mid Carrier ASX00055.
- `bus=0`: Mid Carrier J14 `CAN0_TX/RX` wired to an Adafruit ADA-5708
  TJA1051T/3 transceiver for monitor/system CAN.
- `bus=1`: Mid Carrier J4 terminal CAN1 through the carrier onboard U2 path for
  drive/control CAN.
- MCP2515/TJA1050 remains a verified bench/legacy profile only, not a production
  controller path.

## Canonical Contracts
- Root routing: `AGENTS.md`
- CSM scoped rules: `board/AGENTS.md`, `board/BRIEF.md`
- VMS scoped rules: `qt/AGENTS.md`, `qt/BRIEF.md`
- Wire contract: `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
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
- Make bus meaning, physical backend, role, bitrate, RX/TX support, and control
  policy capability-driven before Qt/VMS removes hard-coded MCP labels.
- Keep legacy 20-byte files as replay/import compatibility only.

## Verification Baseline
Main build:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_dual_can_basic
```

Hardware/HIL claims require an actual hardware run. A successful build alone is
not HIL evidence.
