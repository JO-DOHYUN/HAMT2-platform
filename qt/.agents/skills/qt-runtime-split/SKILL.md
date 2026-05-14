---
name: qt-runtime-split
description: Use when splitting responsibilities between transport, storage, analysis, evidence, graph, control, operator, or UI state.
---

# qt-runtime-split

## When to use
- runtime boundary work
- controller slimming
- replay/live context separation
- transport or storage ownership changes

## Rules
- one runtime owns one clear responsibility
- UI state never becomes evidence state
- storage never depends on visible page state
- analysis never mutates raw truth

## Procedure
1. read `qt/AGENTS.md`, `qt/BRIEF.md`, `qt/docs/QT_RUNTIME_SPLIT_KO.md`
2. define the exact ownership boundary before editing code
3. move one responsibility set at a time
4. keep protocol/storage compatibility explicit
5. record residual coupling honestly

## Acceptance
- code ownership becomes clearer
- no new hidden dependency from graph/UI back into transport/storage
- replay and live semantics remain distinct
