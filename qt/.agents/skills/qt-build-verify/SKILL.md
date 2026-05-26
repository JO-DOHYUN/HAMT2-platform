---
name: qt-build-verify
description: Use when building this Qt/CMake Windows app, fixing compile/link errors, running ctest, creating portable deploy folders, or doing startup smoke checks. Do not use for harness redesign or graph/replay semantics work unless build verification is the main task.
---

# qt-build-verify

## Start
1. Read `BRIEF.md`.
2. Identify whether the change touches build-risk files.
3. Use the canonical workspace and Windows build shell before running CMake.
4. Choose the smallest verification that proves the changed path.

## Workspace
- Primary implementation/build path is the GitHub monorepo Qt subtree: `C:\Users\JEON0295\Documents\PlatformIO\Projects\J_ArdP7_AM2_CSM\qt`.
- The standalone VSM path `C:\WORKS\VS\turn81_full_buildfix2` is a reference/migration workspace. Do not duplicate edits/builds there unless the user asks for comparison or recovery.

## Commands
Windows rule:
- Do not run bare `cmake --build ...` from ordinary PowerShell for MSVC/Qt builds. It can find `cl.exe` but miss MSVC `INCLUDE`/`LIB`, causing false failures such as missing `type_traits` or `utility`.
- Always run configure/build/test through `VsDevCmd.bat`, or an already initialized Visual Studio developer shell.
- If `cmake` is not on `PATH`, use Visual Studio bundled CMake:
  `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`

Release:
```powershell
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && set CAN_MONITOR_QT_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64&& ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"" --preset vs-release-qt6"
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && set CAN_MONITOR_QT_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64&& ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"" --build --preset build-release"
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"" --test-dir out/build/x64-Release --output-on-failure"
```

Debug:
```powershell
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && set CAN_MONITOR_QT_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64&& ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"" --preset vs-debug-qt6"
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && set CAN_MONITOR_QT_PREFIX_PATH=C:/Qt/6.10.2/msvc2022_64&& ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"" --build --preset build-debug"
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"" --test-dir out/build/x64-Debug --output-on-failure"
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
- Keep token output small: summarize successful configure/build/deploy output; paste only failing command excerpts and final error lines.
- For GitHub Actions after push, one status check is enough unless it failed or the user explicitly asks to wait for the remote gate.

## Output
- commands run
- pass/fail result
- unverified surfaces
- next smallest recovery step if failing
