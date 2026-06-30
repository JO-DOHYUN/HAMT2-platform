# AGENTS.md - Repository Router

## Purpose
This root file is a routing guide for agents working in the standalone CSM
firmware repository. VSM/Qt and Android app work must live in their own
repositories, not inside this project folder.

## Read Order
1. Read `BRIEF.md` for the current repository-level state.
2. For CSM firmware work, read `board/AGENTS.md` and `board/BRIEF.md`.
3. For binary records, transport, replay, or cross-project contracts, read
   `shared/docs/TRANSPORT_AND_RECORDS_KO.md`.
4. For current integration architecture, read
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
- CSM has two explicit artifacts: Passive Product for vehicle-impact-free
  evidence capture, and Full Instrumented for bench/HIL control work. Do not use
  Full Instrumented as passive acceptance evidence.
- Passive Product has a separate two-bus RX candidate env only for field
  diagnosis. It must still compile out host downlink, control, CAN TX, test TX,
  USB reconnect reset, and MCP normal-mode transitions. It is not
  `verified_passive` until hardware safety case and bench verification IDs are
  nonzero.
- CDC session open/close/DTR/re-enumeration must not change MCP mode, transceiver
  mode, TX gate, or reset policy. Host absent service drains CAN front-end RX and
  records discard summary counters; it must not stage typed frame payloads.
- `shared/docs/TRANSPORT_AND_RECORDS_KO.md` is the CSM/VMS wire-contract source
  of truth.
- Do not create or maintain nested `qt/`, `vsm/`, `android/`, or duplicate
  `csm/` project workspaces under this CSM repository. Build and upload CSM only
  from this repository root with PlatformIO.
- Bus labels are profile/capability-driven. The Passive Product default env is
  `portenta_h7_m7_mid_mcp2515_j4_dual_csm_passive`, which exposes the external
  MCP2515/TJA1050 lane as listen-only `bus=0`. The Full Instrumented env keeps
  MCP2515/TJA1050 `bus=0` plus Mid Carrier J4/U2 `bus=1` for bench/HIL RX and
  audited control TX. The earlier dual-internal CAN0/CAN1 + TJA1051 target is
  deferred because this ArduinoCore Portenta profile exposes only one practical
  CAN object.

## Change Rules
- If record wire format changes, update `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
  in the same change.
- If MCP behavior changes, preserve MCP `bus=0`, keep bus role/backend exposed
  through `CAPABILITY`, and do not restore the failed MCP falling-edge INT gate.
- If VSM behavior must change, do that in the standalone VSM repository and keep
  requested, host write, accepted, sent audit, and feedback as separate states.
- If passive lifecycle evidence changes, update `include/protocol/TypedRecords.h`
  and `shared/docs/TRANSPORT_AND_RECORDS_KO.md` in the same change.
