# VMS_ARCHITECTURE_KO

이 문서는 VMS Qt 앱의 typed evidence 중심 런타임 분리 목표다.
제품 원칙은 [[PROJECT_CONSTITUTION_KO]], 프로토콜은 [[TYPED_STREAM_PROTOCOL_V1_KO]], 제어 증거 의미는 [[CONTROL_EVIDENCE_CONTRACT_KO]]를 따른다.

## Current Baseline

현재 VMS에는 `TypedTransportParser`, `TypedRecords`, `StorageRuntime`, `TypedReplayReader`, `SerialWorker`, `ControlCommandEncoder`, `ControlPage` foundation이 존재한다.
`AppController`는 여전히 QML facade, orchestration, live/replay/graph/control 상태를 과도하게 포함한다.

## Target Runtime Boundaries

- `SerialWorker`: COM open/read/write and worker-thread ownership only.
- `TransportSession`: live typed stream state, TX queue, backpressure counters, and typed parser dispatch.
- `TypedTransportParser`: SOF resync, length/CRC validation, seq gap accounting.
- `TypedRecordDecoder`: record payload decode only.
- `BoardConnectionState`: `SerialOpen`, `TypedDetected`, `BoardAlive`, `ControlCapable`, stale/fault state.
- `BoardHealthModel`: `BOARD_HEALTH` and `BOARD_EVENT` counters, drops, queue depth, bus health.
- `ControlIntentModel`: operator target, profile, rate limits, neutral/fault fallback command plan.
- `ControlEvidenceModel`: requested, serial written, accepted, sent audit, feedback, timeout timeline.
- `StorageRuntime`: append-only typed stream storage and index.
- `ReplayRuntime`: typed session replay and legacy `.bin` compatibility replay without mixing meanings.
- `AppController`: QML facade and forwarding layer.

## Live Transport Rule

Live production transport is typed-only.
Legacy 20-byte parser stays for legacy replay/import compatibility and tests.
The live UI must not ask the operator to choose legacy versus typed stream.

## Threading Rule

Serial parsing, transport resync, storage append, and blocking or heavy IO work must not run on the UI thread.
VMS UI receives model updates through bounded batches or dirty-state notifications.

## Control Rule

Control command generation must move toward profiles and evidence correlation.
The current lab `ControlPage` can remain for bring-up, but production success display requires matching `CAN_TX_RAW`.

## Migration Order

1. Lock shared protocol and docs.
2. Add protocol parity tests and fixtures.
3. Make live transport typed-only.
4. Add `BoardConnectionState` and `BoardHealthModel`.
5. Add `ControlEvidenceModel`.
6. Split replay/storage and keep legacy `.bin` compatibility.
7. Harden `ControlRuntime` and then refine UI.
