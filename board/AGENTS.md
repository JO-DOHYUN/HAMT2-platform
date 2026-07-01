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
- Passive Product host-absent/no-replay/quarantine/evidence boundary:
  `board/docs/PASSIVE_PRODUCT_BOUNDARY_AUDIT.md`
- Current verified CAN + voltage raw baseline: `board/docs/CAN_VOLTAGE_BASELINE.md`
- Physical hardware concept, pinout, electrical interface, CAN/encoder carrier design: `board/docs/HARDWARE_FINAL_CONCEPT_KO.md` and the specific hardware sub-doc needed
- Mid Carrier dual CAN migration: `board/docs/HARDWARE_MID_TJA1051_DUAL_CAN_KO.md`
- Encoder electrical input or DBS60E wiring/rate checks: `board/docs/ENCODER_DBS60E_INPUT_KO.md`
- Hardware connection/setup/test/verification only:
  `board/docs/HARDWARE_BRINGUP_GATE_HARNESS_KO.md` first, then
  `board/docs/HARDWARE_TEST_AI_AUX_KO.md`
- Mid Carrier MCP2515 bring-up failure history:
  `board/docs/HARDWARE_BRINGUP_POSTMORTEM_2026_05_15_MID_MCP2515_KO.md`
- HIL/soak/flood acceptance runs only: `board/docs/HIL_RUNBOOK_KO.md`
- Safety state machine or hard safety gate work: `board/docs/SAFETY_STATE_MACHINE_KO.md`

## Invariants
- raw capture and derived values stay separate
- board-local `mono64` is the primary time axis
- control must be auditable: host intent -> board accept -> actual tx -> ack/event
- direct analog samples must never be disguised as CAN frames
- voltage raw samples use `ADC_SAMPLE`; scaling/calibration belongs to host/profile metadata
- final Mid Carrier dual CSM profile uses external MCP2515/TJA1050 RX and
  audited TX as `bus=0`, and Mid Carrier J4/U2 RX and audited TX as `bus=1`
  only in Full Instrumented bench/HIL builds
- Passive Product must compile out host downlink/control/CAN TX/test paths,
  keep MCP2515 listen-only, force the Mid Carrier J4/U2 built-in CAN lane into
  silent monitor RX, disable USB reconnect reset, and advertise
  `vehicle_impact_state=configured_passive` unless hardware and bench safety
  evidence IDs allow `verified_passive`.
- Passive Product must not stage CAN payload while host CDC/DTR is absent.
  Session open must discard stale uplink/session payload, bump session epochs,
  report host-absent summary, and then emit only new capture-session CAN frames.
- USB attach quarantine is not a CAN front-end stop. It only cleans
  CDC/uplink/session state while CAN RX passive drain continues.
- Capability hardware evidence fields are claims/references. They must not be
  used as physical proof without matching external analyzer/scope/DTC artifacts.
- The current product is two-bus only. Keep missing/one-bus mismatch evidence;
  do not reinterpret it as an accepted one-bus passive SKU.
- Passive Product is not an ACK provider. Single-node Kvaser/PCAN transmit tests
  must use a separate active ACK node or an explicit lab ACK/TX profile.
- the dual internal CAN0/CAN1 + TJA1051 direction is deferred; hosts must learn
  labels from `CAPABILITY` instead of hard-coding board assumptions
- overflow or drop without an event/counter is a defect
- production host TX must pass heartbeat, arm, lease, safety inputs, backend
  readiness, and allowlist policy before it reaches a CAN backend
- bus descriptor role is only a hint; System/Drive semantics belong to VMS
  binding, not the board firmware

## Architecture Direction
- board is not a simple bridge anymore
- default product artifact is a vehicle-impact-free passive evidence device;
  active control belongs only to Full Instrumented bench/HIL artifacts
- lab ACK/TX profiles may exist for bench tooling, but they are never vehicle
  passive acceptance artifacts
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
- MCP2515 remains the current working external controller path; J4/U2 uses the
  mbed CAN driver directly as the second physical RX channel so passive builds
  can request silent monitor mode.
- `CONTROL_ACK` means rejected or accepted by the board control path. Actual CAN
  write success remains `CAN_TX_RAW`.

## Delivery Rules
- prefer patch-sized changes unless architecture reset is explicitly needed
- if record schema changes, update shared docs in the same turn
- if queue or timestamp semantics change, update validation docs in the same turn

## Verify Before Claiming
- build or say build not verified
- state which lane changed
- state which risks remain
