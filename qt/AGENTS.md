# AGENTS.md — QT

## Mission
- evidence-first diagnostic application
- keep raw truth separate from analysis and display
- show the operator what is wrong and why

## Source of Truth
1. current QT baseline zip/source
2. `qt/BRIEF.md`
3. task-matched documents from the conditional map below
4. old notes only after current files

Do not bulk-load the shared or QT docs directories. Load detailed docs only when the current task touches that area.

## Conditional Document Map
- Transport/parser/typed records/mono64/drop semantics: `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- Shared acceptance, capability binding, system-level contract: matching files under `shared/docs/`
- Board stream storage and replay contract: `qt/docs/QT_TYPED_RECORD_STORAGE.md`
- Evidence UI or operator-facing proof view: `qt/docs/EVIDENCE_VIEW_KO.md`
- Crash-safe storage or write/recovery semantics: `qt/docs/CRASH_SAFE_STORAGE_KO.md`
- Runtime split or ownership boundaries: `qt/docs/QT_RUNTIME_SPLIT_KO.md`
- Session file format or replay persistence: `qt/docs/SESSION_FILE_FORMAT_KO.md`

## Invariants
- transport, storage, analysis, evidence, graph, control, operator state stay separable
- replay is not live-with-a-slider; it is a real reconstruction context
- graph optimization must not change raw truth
- board sensor direct / controller CAN / command TX must remain distinguishable
- raw `capture.stream` is the source of truth; decoded CAN, voltage scaling, graph data, and operator notes are sidecars
- CAN bus labels must come from `CAPABILITY`; profile major `3` may expose
  Mid Carrier MCP2515/TJA1050 `bus=0` and Mid Carrier J4/U2 `bus=1`, while the
  deferred profile major `2` describes the unproven dual internal CAN/TJA1051
  target.
- voltage `ADC_SAMPLE` must remain separate from CAN evidence
- logging must be crash-safe and operator-clear

## Runtime Target
- `TransportRuntime`
- `StorageRuntime`
- `AnalysisRuntime`
- `EvidenceRuntime`
- `GraphRuntime`
- `ControlRuntime`
- `OperatorRuntime`
- `UiStateStore`

## Delivery Rules
- avoid growing the main controller into a monolith
- if protocol or file format changes, update shared docs in the same turn
- if replay semantics change, state impact on evidence and operator messages

## Verify Before Claiming
- build or say build not verified
- list touched runtime areas
- state remaining risks honestly
