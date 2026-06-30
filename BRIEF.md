# BRIEF.md - CSM Repository Brief

## Current Focus
This is the standalone CSM board firmware repository.

- CSM firmware lives at the repository root, `src/`, `include/`, and `board/`.
- Shared CSM/VSM wire contracts live under `shared/`.
- VSM/Qt and Android app work must remain in separate repositories.
- Build and upload from this directory only:
  `C:\Users\JEON0295\Documents\PlatformIO\Projects\J_ArdP7_AM2_CSM`.

## Current Build Baseline
- Board: Arduino Portenta H7 M7 + Mid Carrier ASX00055.
- Product default env: `portenta_h7_m7_mid_mcp2515_j4_dual_csm_passive`.
- Product candidate env for two-bus RX field diagnosis:
  `portenta_h7_m7_mid_mcp2515_j4_dual_csm_passive_2bus_rx`.
- Bench/HIL full env: `portenta_h7_m7_mid_mcp2515_j4_dual_csm_full_instrumented`.
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
- Passive Product `bus=0`: external MCP2515/TJA1050, 8 MHz crystal, Classic
  CAN 2.0 500 kbps, MCP listen-only. Host downlink/control/TX/test paths are
  compile-time removed. Encoder-derived streaming is also disabled in the
  product passive env; keep non-CAN instrumentation in Full/diagnostic envs.
- Full Instrumented keeps `bus=0` MCP2515/TJA1050 and `bus=1` Mid Carrier J4
  CAN1 through onboard U2 for bench/HIL audited control tests.
- Live production output is typed transport v1.
- High-load receive uses `CAN_RX_SEGMENT` while preserving per-frame truth.
- `CONTROL_ACK` is request evidence only in Full Instrumented; final CAN write
  evidence is `CAN_TX_RAW`. Passive Product must not advertise either as a live
  active capability.
- `CAPABILITY` exposes firmware identity, bus descriptors, and build settings.
- `CAPABILITY v5` exposes firmware profile, vehicle-impact state, host command
  RX, control path, USB reset sensitivity, CDC DTR session policy, and passive
  safety evidence IDs.
- `BOARD_HEALTH v7` preserves earlier offsets and adds uplink pool/descriptor,
  CAN RX task max time, USB reconnect/reset counters, and passive violation
  latch diagnostics plus host-absent discard, MCP passive readback, TXREQ
  violation, and DTR change counters.
- Passive CDC uplink is session-gated: before host CDC/DTR session open, CAN
  front-end service continues, but received frames are discarded by explicit
  host-absent counters and are not staged into typed segment/uplink payload
  pools. Session open immediately re-emits capability, USB CDC session evidence,
  host-absent summary when present, and board health.

## Canonical Contracts
- Root routing: `AGENTS.md`
- Board scoped rules: `board/AGENTS.md`, `board/BRIEF.md`
- Wire contract: `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- HIL runbook: `board/docs/HIL_RUNBOOK_KO.md`

## Verification Commands
Build Passive Product firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm_passive
```

Build Passive Product two-bus RX candidate firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm_passive_2bus_rx
```

Build Full Instrumented firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e portenta_h7_m7_mid_mcp2515_j4_dual_csm_full_instrumented
```

Decode board identity after upload:

```powershell
py -3 pc_tools\verify_typed_stream.py --port COM7 --seconds 4 --max-records 20
```

## Immediate Next Work
- Use this repository root for all CSM PlatformIO build/upload work.
- Keep VSM parser updates in the standalone VSM repository.
- Do not commit build outputs, captures, archives, or nested app workspaces.
