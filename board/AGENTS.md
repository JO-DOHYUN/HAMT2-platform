# AGENTS.md — Board

## Mission
- evidence-first capture board
- never hide drop, overflow, timeout, mismatch
- keep capture truth above convenience

## Source of Truth
1. root `AGENTS.md` / `BRIEF.md` routing context
2. current board source (`src/main.cpp`, split modules, `platformio.ini`)
3. `board/BRIEF.md`
4. task-matched documents from the conditional map below
5. `VMS_CSM_03_ARCHITECT_SYNTHESIS_FINAL.md` as a synthesis decision input
6. old notes only after current files

Do not bulk-load the shared or board docs directories. Load detailed docs only when the current task touches that area.

## Conditional Document Map
- Record schema, typed stream, mono64, drop/overflow contract: `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- Shared acceptance, capability binding, system-level contract: matching files under `shared/docs/`
- Board firmware architecture or lane ownership: `board/docs/BOARD_ARCH_CURRENT_ADDENDUM_KO.md` first, then `board/docs/BOARD_ARCH_KO.md` for historical context
- Current verified CAN + voltage raw baseline: `board/docs/CAN_VOLTAGE_BASELINE.md`
- Physical hardware concept, pinout, electrical interface, CAN/encoder carrier design: `board/docs/HARDWARE_FINAL_CONCEPT_KO.md` and the specific hardware sub-doc needed
- Mid Carrier dual CAN migration: `board/docs/HARDWARE_MID_TJA1051_DUAL_CAN_KO.md`
- Encoder electrical input or DBS60E wiring/rate checks: `board/docs/ENCODER_DBS60E_INPUT_KO.md`
- Hardware connection/setup/test/verification only: `board/docs/HARDWARE_TEST_AI_AUX_KO.md`
- HIL/soak/flood acceptance runs only: `board/docs/HIL_RUNBOOK_KO.md`
- Safety state machine or hard safety gate work: `board/docs/SAFETY_STATE_MACHINE_KO.md`

## Invariants
- raw capture and derived values stay separate
- board-local `mono64` is the primary time axis
- control must be auditable: host intent -> board accept -> actual tx -> ack/event
- direct analog samples must never be disguised as CAN frames
- voltage raw samples use `ADC_SAMPLE`; scaling/calibration belongs to host/profile metadata
- current MCP bench baseline uses MCP RX as `bus=0` and Portenta built-in CAN TX
  as `bus=1`
- Mid Carrier production target uses internal CAN0 + TJA1051 as `bus=0` and Mid
  Carrier terminal CAN1 as `bus=1`; hosts must learn labels from `CAPABILITY`
- overflow or drop without an event/counter is a defect

## Architecture Direction
- board is not a simple bridge anymore
- target shape:
  - `CanRxLane`
  - `EncoderEdgeLane`
  - `AnalogSampleLane`
  - `CanTxAuditLane`
  - `ControlLane`
  - `UplinkScheduler`
  - `SafetySupervisor`
  - `HealthMonitor`
- `src/main.cpp` should shrink toward setup/loop orchestration; protocol,
  record helpers, host parsing, lane IO, control, safety, health, and capability
  logic should move into scoped modules without changing the typed wire contract.
- MCP2515 code is legacy/bench only in the Mid Carrier production target.

## Delivery Rules
- prefer patch-sized changes unless architecture reset is explicitly needed
- if record schema changes, update shared docs in the same turn
- if queue or timestamp semantics change, update validation docs in the same turn

## Verify Before Claiming
- build or say build not verified
- state which lane changed
- state which risks remain
