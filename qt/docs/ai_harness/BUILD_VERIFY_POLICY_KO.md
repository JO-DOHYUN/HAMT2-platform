# BUILD_VERIFY_POLICY_KO

이 문서는 VMS-CSM 통합 작업 중 Codex가 보고해야 하는 빌드/검증 기준이다.
상위 운영 맵은 [[../../AGENTS]], 현재 기준은 [[../../BRIEF]]를 따른다.

## VMS Baseline

- Windows + MSVC 2022 x64
- Qt 6.10.2 baseline
- `CMakePresets.json` and local `CMakeUserPresets.json`
- `CAN_MONITOR_QT_PREFIX_PATH` or existing local Qt preset path

## Standard Commands

```powershell
cmake --preset local-vs-release-qt6
cmake --build --preset build-release-local
ctest --test-dir out/build/x64-Release --output-on-failure
```

Portable smoke when deploy behavior is touched:

```powershell
scripts\deploy_release.bat out\build\x64-Release out\build\x64-Release\portable_check
```

## Reporting Rule

- Do not claim build/test/run success unless the command was run in the current turn.
- If only docs/protocol markdown changed, build may be skipped, but link/path/static consistency checks should be reported.
- If `AppController`, `SerialWorker`, `TypedRecords`, CMake, QML registration, or any new C++ file changes, run build and relevant tests.
- If HIL is not physically performed, state it as unverified.

## Build-Risk Surfaces

- `AppController.h/.cpp`
- `SerialWorker.h/.cpp`
- `TypedRecords.h/.cpp`
- `TypedTransportParser.h/.cpp`
- `ControlCommandEncoder.h/.cpp`
- CMake/QML module/import/type registration
- new signal/property/slot or `Q_PROPERTY`
