# Requirements Traceability

| Requirement ID | Requirement | Current Owner | Automated Coverage | Manual Acceptance |
| --- | --- | --- | --- | --- |
| RQ-BUILD-001 | Shared preset configure/build path exists | CMake | CI configure/build | fresh-shell configure/build |
| RQ-TEST-001 | `ctest` runs real tests | tests/ | `packet_parser`, `model_pack_validator` | n/a |
| RQ-MODEL-001 | malformed model pack is rejected | `ModelPackValidator` | `model_pack_validator` | load invalid external model |
| RQ-LOG-001 | session logs are written to standard app path | `AppLogging`, `RuntimePaths` | smoke through startup | inspect generated log file |
| RQ-GRAPH-001 | overview/detail semantics preserved | `AppController`, QML, `GraphViewportItem` | pending | operator graph smoke |
| RQ-REPLAY-001 | replay seek/speed semantics preserved | `AppController`, `ReplayEngine` | pending | replay operator smoke |
