# QT_TYPED_RECORD_STORAGE

## Role

Qt is the evidence workstation. It should receive the board typed stream and preserve the raw binary truth first. Decoding, graphs, CAN signal interpretation, voltage scaling, and operator UI are derived layers.

## Required Data Flow

1. `TransportRuntime`
   - reads USB serial bytes
   - resynchronizes on `0xA5 0x5A`
   - validates length and CRC
   - dispatches typed records without changing payload bytes

2. `StorageRuntime`
   - appends the exact framed bytes to `capture.stream.part`
   - writes a sparse index keyed by board `mono_us`
   - finalizes with atomic rename after a clean stop

3. `AnalysisRuntime`
   - decodes CAN using the selected profile/database
   - interprets VCU encoder feedback from CAN first
   - converts `ADC_SAMPLE` raw counts using the active voltage calibration profile

4. `EvidenceRuntime`
   - exposes source identity from `CAPABILITY`, not hard-coded bus names
  - profile major `3`: current Mid Carrier CSM. `bus=0` is
    MCP2515/TJA1050; `bus=1`, when present, is Mid Carrier J4/U2. Display
    system/monitor or drive/control from each bus descriptor role, not from the
    bus number.
  - profile major `2`: deferred dual internal CAN/TJA1051 target
  - profile major `1`: legacy MCP RX `bus=0`, built-in CAN RX/TX `bus=1`, voltage ADC source `0`
   - shows drop/fault counters from board records
   - never hides raw stream mismatch, CRC loss, queue drop, or replay gaps

## Session Files

Minimum session set:
- `capture.stream.part`: append-only original typed record bytes
- `capture.index.part`: sparse binary index, record offset and `mono_us`
- `session.meta.json`: board capability, selected CAN DB/profile, voltage calibration, start/stop wall time
- `events.jsonl`: operator notes and host-side state changes

The final file names drop `.part` only after all buffers are flushed.

## Current Record Priorities

Qt must handle these now:
- `CAN_RX_RAW`: raw CAN receive frames with bus role/backend resolved from `CAPABILITY`
- `CAN_TX_RAW`: actual board CAN TX audit frames
- `ADC_SAMPLE`: voltage raw samples
- `BOARD_HEALTH`: state, counters, fault bits
- `BOARD_EVENT`: boot, failure, and diagnostic events
- `CAPABILITY`: version and lane support

Encoder direct records remain supported by contract but are not the immediate implementation focus.
