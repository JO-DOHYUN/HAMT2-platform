# AGENTS.md - Repository Router

## Purpose
This root file is a routing guide for agents. It does not replace the scoped
rules under `board/`, `qt/`, or `shared/`. This branch freezes the current CSM
baseline before VMS integration; avoid broad repository restructuring here.

## Read Order
1. Read `BRIEF.md` for the current repository-level state.
2. For CSM firmware work, read `board/AGENTS.md` and `board/BRIEF.md`.
3. For VMS/Qt work, read `qt/AGENTS.md` and `qt/BRIEF.md`.
4. For binary records, transport, replay, or cross-project contracts, read
   `shared/docs/TRANSPORT_AND_RECORDS_KO.md`.
5. For current integration architecture, read
   `VMS_CSM_03_ARCHITECT_SYNTHESIS_FINAL.md` as a decision input, then apply it
   through the scoped docs instead of treating it as a replacement prompt.

## Routing Rules
- Do not bulk-load every document. Load only the task-matched docs listed in the
  scoped AGENTS files.
- Hardware wiring/setup/testing/verification work must use
  `board/docs/HARDWARE_BRINGUP_GATE_HARNESS_KO.md` first, then may use
  `board/docs/HARDWARE_TEST_AI_AUX_KO.md`. General code or document work should
  not load those hardware-only auxiliary files.
- The live production stream is typed transport v1 only. Legacy 20-byte data is
  replay/import compatibility, not a live output mode.
- `CONTROL_ACK` is board request evidence. Actual CAN TX success is proven only
  by `CAN_TX_RAW`.
- CSM is a typed evidence and control gateway, not a simple USB-CAN bridge.
- `shared/docs/TRANSPORT_AND_RECORDS_KO.md` is the CSM/VMS wire-contract source
  of truth.
- Bus labels are profile/capability-driven. The final Mid Carrier dual CSM env
  uses external MCP2515/TJA1050 `bus=0` plus Mid Carrier J4/U2 `bus=1` for typed
  RX and audited control TX. The earlier dual-internal CAN0/CAN1 + TJA1051
  target is deferred because this ArduinoCore Portenta profile exposes only one
  practical CAN object.

## Change Rules
- If record wire format changes, update `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
  in the same change.
- If MCP behavior changes, preserve MCP `bus=0`, keep bus role/backend exposed
  through `CAPABILITY`, and do not restore the failed MCP falling-edge INT gate.
- If Qt/VMS behavior changes, keep requested, accepted, sent audit, and feedback
  as separate states.
