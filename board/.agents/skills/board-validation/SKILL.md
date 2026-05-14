---
name: board-validation
description: Use when preparing HIL, flood, pulse injection, ADC burst, reconnect, or soak validation for the board.
---

# board-validation

## Purpose
Detect hidden loss, timing drift, unsafe control behavior, and cross-lane starvation.

## Validation Set
- CAN flood
- encoder pulse injection
- ADC burst with concurrent CAN load
- reconnect / lease timeout / estop
- power-loss recovery
- long soak

## Rules
- validation must measure evidence quality, not just functionality
- any loss must be localized to lane and time range
- if exact measurement is unavailable, say so

## Output
- test command or setup
- expected result
- observed limitation
- release blocker or non-blocker classification
