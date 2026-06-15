# BRIEF.md - CSM Repository Brief

## Current Focus
This is the standalone CSM board firmware repository.

- CSM firmware lives at the repository root, `src/`, `include/`, and `board/`.
- Shared CSM/VSM wire contracts live under `shared/`.
- VSM/Qt and Android app work must remain in separate repositories.
- Build and upload from this directory only:
  `C:\Users\JEON0295\Documents\PlatformIO\Projects\J_ArdP7_AM2_CSM`.

## Current Uploaded Baseline
- Board: Arduino Portenta H7 M7 + Mid Carrier ASX00055.
- Production env: `portenta_h7_m7_mid_mcp2515_j4_dual_csm`.
- Current board identity before this cleanup was read on COM7 as:
  - `git=4d21a3c9431`
  - `dirty=1`
  - `env=portenta_h7_m7_mid_mcp2515_j4_dual_csm`
  - `BOARD_CAN_IRQ_MODE=0`
  - `BOARD_MCP2515_SPI_HZ=8000000`
  - `BOARD_CAN_SERIAL_DRAIN_BUDGET=512`
- This repository now carries that uploaded source state so future uploads no
  longer depend on `C:\WORKS\VS\csm_zip_pre_wifi`.

## CSM Baseline
- `bus=0`: external MCP2515/TJA1050, 8 MHz crystal, Classic CAN 2.0 500 kbps.
- `bus=1`: Mid Carrier J4 CAN1 through onboard U2, Classic CAN 2.0 500 kbps.
- Live production output is typed transport v1.
- High-load receive uses `CAN_RX_SEGMENT` while preserving per-frame truth.
- `CONTROL_ACK` is request evidence only; final CAN write evidence is
  `CAN_TX_RAW`.
- `CAPABILITY` exposes firmware identity, bus descriptors, and build settings.
- `BOARD_HEALTH v4` exposes bus queue/drop, serial backpressure, and MCP drain
  diagnostics.

## Canonical Contracts
- Root routing: `AGENTS.md`
- Board scoped rules: `board/AGENTS.md`, `board/BRIEF.md`
- Wire contract: `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- HIL runbook: `board/docs/HIL_RUNBOOK_KO.md`

## Verification Commands
Build production firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm
```

Upload production firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm -t upload
```

Decode board identity after upload:

```powershell
py -3 pc_tools\verify_typed_stream.py --port COM7 --seconds 4 --max-records 20
```

## Immediate Next Work
- Use this repository root for all CSM PlatformIO build/upload work.
- Keep VSM parser updates in the standalone VSM repository.
- Do not commit build outputs, captures, archives, or nested app workspaces.
