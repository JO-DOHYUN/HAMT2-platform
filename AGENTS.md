# AGENTS.md - Repository Router

## Purpose
This root file is a routing guide for agents. It does not replace the scoped
rules under `board/`, `qt/`, or `shared/`.

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
- Hardware wiring/setup/testing/verification work may use
  `board/docs/HARDWARE_TEST_AI_AUX_KO.md`. General code or document work should
  not load that hardware-only auxiliary file.
- The live production stream is typed transport v1 only. Legacy 20-byte data is
  replay/import compatibility, not a live output mode.
- `CONTROL_ACK` is board request evidence. Actual CAN TX success is proven only
  by `CAN_TX_RAW`.
- CSM is a typed evidence and control gateway, not a simple USB-CAN bridge.
- Bus labels are profile/capability-driven. The verified MCP bench profile uses
  MCP `bus=0` and built-in CAN `bus=1`; the Mid Carrier production target uses
  internal CAN0 + TJA1051 `bus=0` and Mid Carrier terminal CAN1 `bus=1`.

## Change Rules
- If record wire format changes, update `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
  in the same change.
- If legacy MCP bench behavior changes, preserve MCP `bus=0`, built-in CAN
  `bus=1`, and do not restore the failed MCP falling-edge INT gate. If Mid
  Carrier production behavior changes, keep MCP2515 out of the production path
  and expose bus role/backend through `CAPABILITY`.
- If Qt/VMS behavior changes, keep requested, accepted, sent audit, and feedback
  as separate states.
