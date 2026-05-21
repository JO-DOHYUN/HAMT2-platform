---
name: qt-build-verify
description: Use when building this Qt/CMake Windows app, fixing compile/link errors, running ctest, creating portable deploy folders, or doing startup smoke checks. Do not use for harness redesign or graph/replay semantics work unless build verification is the main task.
---

# qt-build-verify

## Start
1. Read `BRIEF.md`.
2. Identify whether the change touches build-risk files.
3. Choose the smallest verification that proves the changed path.

## Commands
Release:
```powershell
cmake --preset local-vs-release-qt6
cmake --build --preset build-release-local
ctest --test-dir out/build/x64-Release --output-on-failure
```

Debug:
```powershell
cmake --preset local-vs-debug-qt6
cmake --build --preset build-debug-local
ctest --test-dir out/build/x64-Debug --output-on-failure
```

Portable:
```powershell
scripts\deploy_release.bat out\build\x64-Release out\build\x64-Release\portable_typed_check
```

## Rules
- Do not claim build/test/deploy success unless the command was run in this turn or clearly inherited as historical state.
- Keep unrelated UI/docs cleanup out of build-fix turns.
- If `QTP0004` appears, report it as known dev warning unless it blocks generation.
- If `CMakeLists.txt`, QML registration, or new C++ files changed, verify declaration/definition/CMake/QML registration together.

## Output
- commands run
- pass/fail result
- unverified surfaces
- next smallest recovery step if failing
