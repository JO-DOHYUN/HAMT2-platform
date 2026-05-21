# BRIEF.md - Repository Brief

## Current Focus
This repository is now the integrated HAMT2 CSM/VSM platform workspace.

- CSM board firmware lives at the repository root and `board/`.
- VSM Qt workstation source lives in `qt/`.
- Shared binary and architecture contracts live in `shared/` and `docs/architecture/`.
- GitHub target: `JO-DOHYUN/HAMT2-platform`.
- Active branch: `codex/baseline-before-vms`.

## CSM Baseline
- Arduino Portenta H7 M7 + Portenta Mid Carrier ASX00055, PlatformIO, Arduino framework.
- External MCP2515/TJA1050 lane is `CAN_RX_RAW bus=0` and audited host TX lane.
- Mid Carrier J4/U2 lane is `CAN_RX_RAW bus=1` and audited host TX lane in `portenta_h7_m7_mid_mcp2515_j4_dual_csm`.
- USB typed binary stream v1 with CRC16 framing.
- Host downlink commands: `HOST_CAN_TX_REQUEST`, `HOST_HEARTBEAT`, `HOST_CONTROL_SESSION`.
- CSM emits `CONTROL_ACK` for board decision and `CAN_TX_RAW` only after actual CAN write success.
- Voltage raw lane is `ADC_SAMPLE`, not fake CAN.

## VSM Baseline
- VSM Qt/C++ source is now present under `qt/`.
- VSM live production path is typed evidence stream only.
- Legacy 20-byte `.bin` remains replay/import compatibility.
- VSM replay supports both legacy `.bin` and typed capture sessions (`typed_capture_*.typed/capture.stream`).
- Current Qt verification baseline from the source workspace: Release build passed, `ctest` passed 19/19, startup smoke passed.

## Canonical Contracts
- Root routing: `AGENTS.md`
- CSM scoped rules: `board/AGENTS.md`, `board/BRIEF.md`
- VSM scoped rules: `qt/AGENTS.md`, `qt/BRIEF.md`
- Wire contract source of truth: `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- VSM local shared protocol mirror: `qt/shared/protocol/typed_stream_v1.md`
- Integration decision input: `VMS_CSM_03_ARCHITECT_SYNTHESIS_FINAL.md`
- VSM completion plan: `qt/docs/COMPLETION_TO_RELEASE_PLAN_KO.md`

## Non-Negotiable Evidence Rule
The control evidence chain is:

`host intent -> host write -> CONTROL_ACK accept/reject -> actual CAN write -> CAN_TX_RAW audit -> optional feedback CAN_RX_RAW`

`CONTROL_ACK` alone is never final CAN TX success.

## Verification Commands
CSM firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm
```

VSM Qt:

```powershell
cd qt
$env:CAN_MONITOR_QT_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64"
cmake --preset vs-release-qt6
cmake --build --preset build-release
ctest --preset test-release
```

## Immediate Next Work
- Verify VSM from the monorepo `qt/` location.
- Verify active CSM PlatformIO build.
- Commit and push the integrated CSM/VSM state to GitHub.
- Continue VSM completion through `qt/docs/COMPLETION_TO_RELEASE_PLAN_KO.md`.
