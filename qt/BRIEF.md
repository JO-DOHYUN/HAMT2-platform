# BRIEF.md — QT

## Current Baseline
- baseline source: `qt/docs/*`; no Qt C++ source is present in this repository snapshot
- protocol/file format version: board framed typed records v1, see `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- verified build status: not applicable in this repository snapshot
- verified replay/storage status: architecture only; raw capture can be exercised with `pc_tools/log_capture.py`

## This Turn Goal
- main goal: store one ordered binary stream containing CAN RX, CAN TX audit, voltage raw ADC, health, and events
- non-goals: direct DBS60E encoder implementation; VCU encoder feedback is interpreted from CAN first

## Working Features To Preserve
- raw packet parsing remains truthful
- replay time axis remains meaningful
- graph optimization never rewrites truth
- board raw stream remains append-only and byte-for-byte recoverable

## Acceptance
- storage semantics preserved or migration documented
- replay/evidence parity checked
- operator-visible state remains unambiguous
- build-risk noted if relevant
- `CAN_RX_RAW`, `CAN_TX_RAW`, and `ADC_SAMPLE` remain distinguishable in files and UI

## Risks
- open items: Qt implementation source is not present here; only architecture docs can be updated in this repo
- manual checks: serial reconnect, CRC resync, sparse index recovery, replay parity
