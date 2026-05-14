---
name: qt-evidence-graph
description: Use when editing evidence overlays, compare views, graph caching, sampling, or cursor alignment.
---

# qt-evidence-graph

## Goal
Show comparable truth on one time axis without distorting source meaning.

## Required Lanes
- sensor direct
- controller CAN
- command TX
- board events

## Rules
- display cache is not truth
- downsample must preserve peaks and event visibility
- evidence cursor must align by mono64, not page-local assumptions
- source badges must stay visible

## Acceptance
- operator can tell whether the sensor, the controller message, or the command path diverged
- graph remains usable under long sessions
- no hidden smoothing that changes diagnosis
