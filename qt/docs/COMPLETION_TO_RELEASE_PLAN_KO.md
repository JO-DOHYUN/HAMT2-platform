# VSM Completion To Release Plan

## 1. Current Position

VSM은 기능 데모가 아니라 CSM typed evidence/control gateway를 기준으로 동작하는 Windows 현장 검증 workstation으로 정리한다.

현재 완료된 기반:
- Qt/CMake Release build and `ctest` gate: 19/19 passing.
- Legacy `.bin` replay/import compatibility.
- Typed live parser, typed record decode, append-only typed storage foundation.
- Typed replay reader and typed capture session loading through `capture.stream`.
- Control command encoder, host heartbeat/session command, VCU command burst, slew limiter, and lab ControlPage foundation.
- Board alive/control evidence doctrine: COM open, `CONTROL_ACK`, `CAN_TX_RAW`, and feedback are separate states.

현재 기준 소스는 이 폴더이며, GitHub 통합 위치는 `HAMT2-platform/qt/`이다.

## 2. Completion Definition

완성 판정은 다음 gate를 모두 통과해야 한다.

- VSM clean build, `ctest`, startup smoke, portable deploy smoke pass.
- CSM PlatformIO build pass for the active dual-channel profile.
- VSM can load current model baseline and decode live/replay CAN.
- VSM live connection does not call board alive until `CAPABILITY` and fresh `BOARD_HEALTH` are present.
- Control UI never calls `CONTROL_ACK` actual success; actual success requires matching `CAN_TX_RAW`.
- Typed recording can be replayed from `.typed/capture.stream`; legacy `.bin` still works.
- Bus role labels are resolved from capability/model/observed fingerprints/operator override, not hard-coded.
- ControlPage is Korean, stable, readable, and separates intent, queued, accepted, sent audit, feedback, and fault.
- GitHub repository contains both CSM firmware and VSM Qt source with scoped AGENTS/BRIEF routing.

## 3. Remaining Work Packages

### P0: Repository And Release Hygiene
- Publish VSM source into `HAMT2-platform/qt/`.
- Update root, board, qt, and shared routing docs so Codex can work from the monorepo without losing context.
- Add root GitHub workflow for Qt build/test from `qt/`.
- Keep generated artifacts out of git: `.pio`, `out`, `.vs`, logs, captures, deploy folders, `.bin`, `.stream`, `CMakeUserPresets.json`.

Acceptance:
- `git status` contains only intended source/docs after sync.
- CSM and VSM builds can be started from documented commands.

### P1: Transport And Evidence Runtime Split
- Move live serial ownership out of `AppController` into `TransportRuntime`.
- Keep `SerialWorker` as byte-source/worker owner only.
- Add bounded typed event dispatch into `EvidenceRuntime`.
- Add `BoardHealthModel` and expose health stale, parser faults, queue depth, drops, and fault flags.

Acceptance:
- No UI-thread parsing/blocking writes.
- COM open alone is displayed as serial-open only, not board-alive.
- Health stale/fault is visible in UI and logs.

### P2: Control Audit Model And Safety Gate
- Add `ControlAuditModel` for `HostIntent -> HostWrite -> CONTROL_ACK -> CAN_TX_RAW -> Feedback CAN_RX_RAW`.
- Make ControlPage operator-facing Korean UI, not rapid English telemetry noise.
- Enforce arm/session/heartbeat/model-valid/role-resolved/allowlist gates before TX.
- Keep neutral hold and slew/rate limits model-backed and visible.

Acceptance:
- Missing `CAN_TX_RAW` never shows "sent success".
- ACK reject reasons are stable and readable.
- Keyboard control angle changes are ramped, not step-jumped.

### P3: Replay And Storage Completeness
- Promote typed session replay beyond CAN_RX projection: index/meta/events/timeline parity.
- Preserve raw typed stream bytes as truth and derived analysis as sidecar.
- Add typed replay parity tests for record count, offsets, seq, type counts, first/last `mono_us`.
- Add recovery behavior for `.part` capture sessions.

Acceptance:
- Typed capture session opens from folder or `capture.stream`.
- Legacy `.bin` replay remains green.
- Bad/partial typed files produce diagnostics, not silent success.

### P4: Model Pack V2 And Bus Role Binding
- Extend model pack with `bus_role_rules`, control whitelist, neutral frames, diagnostics, graph presets, and alarm display policy.
- Bind `BusRoleResolver` to CSM `CAPABILITY` descriptors and model rules.
- Keep current turn77 model as v1 compatibility baseline until v2 is ready.

Acceptance:
- Control TX is denied while role unresolved.
- UI labels System/Drive only after evidence-backed resolution.

### P5: Performance, UI, And Field Gate
- Add operator-first layout pass for Control, BoardHealth, Replay, Full Graph.
- Add high-rate RX load smoke and control TX audit regression script.
- Keep graph overview fixed truth and detail rebuild separated.
- Add release checklist with artifact hash, model hash, protocol version, and known issues.

Acceptance:
- Pcan/Kvaser two-channel synthetic load does not lose typed parser records.
- Release folder starts clean and loads bundled model.

## 4. Verification Commands

VSM from standalone workspace:
```powershell
cmake --preset local-vs-release-qt6
cmake --build --preset build-release-local
ctest --test-dir out/build/x64-Release --output-on-failure
```

VSM from GitHub monorepo `qt/` folder:
```powershell
$env:CAN_MONITOR_QT_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64"
cmake --preset vs-release-qt6
cmake --build --preset build-release
ctest --preset test-release
```

CSM firmware:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm
```

HIL manual gate:
- CSM emits `CAPABILITY`.
- CSM emits fresh `BOARD_HEALTH`.
- VSM sees bus0/bus1 `CAN_RX_RAW`.
- VSM sends host command and receives `CONTROL_ACK`.
- Actual send is accepted only after matching `CAN_TX_RAW`.
- External feedback is checked through separate `CAN_RX_RAW`.

## 5. Risks Held

- Do not claim vehicle control completion from Pcan/Kvaser bench only.
- Do not remove legacy `.bin` replay/import.
- Do not hard-code bus0/bus1 as System/Drive.
- Do not treat voltage/health/control ack as fake CAN frames.
- Do not add closed-loop control before model-backed safety policy and feedback scale are locked.
- Do not bury CSM firmware dirty changes when publishing VSM; commit them explicitly or leave them visible.
