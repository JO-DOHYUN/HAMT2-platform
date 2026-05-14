---
name: qt-storage-replay
description: Use when changing capture files, metadata, indexes, replay loading, replay seeking, or crash-safe save/finalize behavior.
---

# qt-storage-replay

## Rules
- raw stream is the source of truth
- replay works from stored evidence, not current live assumptions
- `.part` / temp semantics must remain recoverable
- metadata writes must be atomic

## Procedure
1. inspect current session file contract
2. preserve append-only raw path when possible
3. keep index rebuild path available
4. document migration if file format changes

## Acceptance
- interrupted session can be recovered or clearly marked incomplete
- replay cursor and evidence view agree on time source
- operator can tell recording/finalize state clearly
