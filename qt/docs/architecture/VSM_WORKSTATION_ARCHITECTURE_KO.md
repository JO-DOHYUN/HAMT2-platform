# VSM Workstation Architecture

이 문서는 `VSM_03_architect_synthesis_codex_prompt.md`를 구현 기준으로 삼는 VSM 재구성의 현재 canonical architecture note다.

## Product Bar

VSM은 기존 Qt CAN monitor가 아니라 CSM typed evidence stream을 truth로 삼는 evidence-first CAN control workstation이다.

항상 유지할 기준:

- live production stream은 typed evidence 전용이다.
- legacy 20-byte는 replay/import compatibility 경로로만 유지한다.
- COM open은 board alive가 아니다.
- board alive는 valid `CAPABILITY`와 fresh `BOARD_HEALTH`를 기준으로 판단한다.
- `CONTROL_ACK`는 command acceptance evidence일 뿐이다.
- 실제 CAN 송신 성공은 matching `CAN_TX_RAW` audit evidence만 기준으로 한다.
- ADC, health, event, control ack는 fake CAN frame으로 만들지 않는다.
- bus number는 role이 아니다. role은 capability, model rule, observed fingerprint, operator override로 resolution한다.

## Runtime Spine

```text
TransportRuntime
  -> EvidenceRuntime
  -> StorageRuntime
  -> ReplayRuntime
  -> AnalysisRuntime / GraphRuntime
  -> ControlRuntime
  -> QML facade
```

`AppController`는 이 spine을 QML에 노출하는 facade로 축소한다. Serial parsing, storage write, replay decode, graph rebuild, control safety state machine은 AppController 내부에 직접 누적하지 않는다.

## Current Implementation Step

이번 단계에서 고정한 첫 기반:

- `TypedRecords`에 `CONTROL_ACK` payload decode를 추가했다.
- `BoardConnectionState`를 추가해 serial open, capability, health, protocol, alive, control capable을 분리했다.
- `BusRoleResolver`를 추가해 bus number hardcoding 없이 role resolution과 TX allow 판단을 시작했다.
- `TransportRuntime` skeleton을 추가해 live production typed-only enforcement 지점을 만들었다.
- `HostTxQueue`를 추가해 host command TX를 non-blocking FIFO/backpressure 구조로 전환했다.
- AppController control arm/send path는 board evidence gate를 통과하지 못하면 막히도록 1차 연결했다.

## Next Implementation Step

다음 단계의 우선순위:

1. `TransportRuntime`을 실제 `SerialWorker` ownership으로 확장하고 AppController의 transport orchestration을 줄인다.
2. live production UI에서 legacy/typed 선택을 operator path에서 제거하고 developer diagnostic으로 격하한다.
3. `CONTROL_ACK`, `CAN_TX_RAW`, feedback `CAN_RX_RAW` correlation model을 별도 `ControlAuditModel`로 분리한다.
4. bus role resolver를 model pack v2 `bus_role_rules`와 capability bus descriptors에 연결한다.
5. typed session writer/index/meta/events 구조를 `capture_*.typed` contract로 확장한다.

## Verification Baseline

현재 단계 acceptance:

- Release configure/build 통과.
- 전체 `ctest` 17개 통과.
- portable deploy folder 생성 통과.
- portable exe startup smoke 5초 통과.

관련 문서:

- [[docs/architecture/PROJECT_CONSTITUTION_KO]]
- [[docs/architecture/TYPED_STREAM_PROTOCOL_V1_KO]]
- [[docs/architecture/CONTROL_EVIDENCE_CONTRACT_KO]]
- [[shared/protocol/typed_stream_v1]]
