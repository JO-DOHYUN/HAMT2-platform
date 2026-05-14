# QT_RUNTIME_SPLIT_KO

## 목표
현재형 앱을 양산형 진단기로 끌어올리기 위해, 런타임 책임을 분리한다.

## 권장 분리
- `TransportRuntime`: framed transport, parsing, demux
- `StorageRuntime`: append-only stream, index, meta, finalize
- `AnalysisRuntime`: decode, derived metrics, alarms
- `EvidenceRuntime`: direct vs controller vs command 비교
- `GraphRuntime`: cache, downsample, viewport, overlays
- `ControlRuntime`: host intent, heartbeat, lease, ack matching
- `OperatorRuntime`: root cause, source trust, action text
- `UiStateStore`: purely UI state

## 핵심 원칙
- replay는 별도 문맥
- graph는 표시 최적화 레이어
- 저장 경로는 UI 상태에 종속되면 안 됨
