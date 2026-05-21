# BRIEF.md

## Current Baseline
- Workspace: `C:\WORKS\VS\turn81_full_buildfix2`
- Git status: this VSM workspace is not a git repository; GitHub publishing is done through `C:\Users\JEON0295\Documents\PlatformIO\Projects\J_ArdP7_AM2_CSM`.
- GitHub target: `JO-DOHYUN/HAMT2-platform`, branch `codex/baseline-before-vms`.
- Board source: `C:\Users\JEON0295\Documents\PlatformIO\Projects\J_ArdP7_AM2_CSM`.
- Active model/rules baseline: `data/vms_model_turn77_system_drive_merged_realcan_refresh2_final.json`.
- Product direction: VSM is an evidence-first CAN monitor/logger/replay/decode/control workstation paired with the CSM typed evidence/control gateway.

## Must Preserve
- VMS live production path is CSM typed evidence stream only.
- COM open is not board alive; valid `CAPABILITY` and fresh `BOARD_HEALTH` are required for alive/control-capable state.
- Legacy 20-byte packet, CRC8, DLC, and `t_us` wrap are preserved for replay/import compatibility only.
- Typed evidence separation is mandatory: `CAN_RX_RAW`, `CAN_TX_RAW`, `ADC_SAMPLE`, `BOARD_HEALTH`, `BOARD_EVENT`, `CONTROL_ACK`, and `CAPABILITY`.
- Qt command write, `CONTROL_ACK`, `CAN_TX_RAW`, and feedback remain separate evidence; actual CAN TX success requires matching `CAN_TX_RAW`.
- Live and replay source semantics remain separate.
- Graph truth-first behavior, fixed-axis overview, peak meaning, nested zoom/back/root/clear remain intact.
- Current model/rules decode and control allowlist assumptions must remain traceable.

## Current Verified State
- Release configure/build passed with Qt 6.10.2 + MSVC2022.
- `ctest --test-dir out/build/x64-Release --output-on-failure` passed: 19/19.
- Build exe startup smoke passed for 5 seconds.
- Typed foundation exists: parser, records, storage, typed replay reader, typed replay projection into legacy analysis frames.
- Replay now accepts both legacy `.bin` and typed capture sessions (`typed_capture_*.typed/capture.stream`).
- Control foundation exists: heartbeat/session, 0x503 and 0x510/0x512/0x511/0x513 command burst, slew limiter, host TX queue, and evidence separation.
- CSM HIL from prior run showed bus0/bus1 RX and strict control TX audit matching, but any new hardware claim requires a fresh hardware run.

## Current Goal
- Publish the VSM source into the CSM GitHub repository under `qt/`.
- Keep CSM and VSM contracts aligned around typed stream v1 and control evidence.
- Freeze the remaining-to-complete plan before the next large implementation pass.

## Immediate Next Work
- Finish GitHub sync: copy this VSM source into the CSM repo `qt/`, update monorepo routing docs, build VSM from that location, build CSM firmware, commit, and push.
- After sync, continue implementation in this order:
- Split AppController into runtime-owned backends.
- Add dedicated `ControlAuditModel` for requested/write/ack/audit/feedback correlation.
- Bind `BusRoleResolver` to CSM `CAPABILITY` bus descriptors and model pack rules.
- Promote ControlPage from lab bring-up to Korean operator workflow with clear safety/evidence wording.
- Add typed session sidecar/replay parity coverage for index/meta/event timelines beyond CAN_RX projection.

## Read Next
- Project map: [[INDEX]]
- Completion plan: [[docs/COMPLETION_TO_RELEASE_PLAN_KO]]
- Production plan: [[docs/PLAN]]
- VSM architecture: [[docs/architecture/VSM_WORKSTATION_ARCHITECTURE_KO]]
- VMS runtime split: [[docs/architecture/VMS_ARCHITECTURE_KO]]
- Typed protocol: [[docs/architecture/TYPED_STREAM_PROTOCOL_V1_KO]]
- Control evidence contract: [[docs/architecture/CONTROL_EVIDENCE_CONTRACT_KO]]
- Build runbook: [[docs/runbooks/BUILD_AND_VERIFY_KO]]
