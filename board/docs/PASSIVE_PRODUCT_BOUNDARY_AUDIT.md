# CSM Passive Product Boundary Audit

This document is the firmware-side acceptance boundary for the two-bus Passive
Product CSM.

## Required Runtime Data Flow

```text
vehicle CAN bus0/bus1
  -> passive CAN front-end drain
  -> if host absent: discard payload and increment host-absent counters
  -> if host session open: CAN_RX_SEGMENT evidence for new frames only
  -> CDC evidence uplink
```

Host absent frames are outside the VSM capture session. They must not be staged
and replayed when USB reconnects.

## Session Contract

- `host_session_epoch` increments once per new CDC/DTR host session.
- `transport_epoch` increments with the uplink session epoch.
- Session close discards queued/staged uplink payload.
- Session open discards stale uplink payload, resets the segment builder epoch,
  emits `CAPABILITY`, emits USB session evidence, emits host-absent summary if
  present, then emits board health.
- `USB_ATTACH_QUARANTINE` is CDC/uplink/session cleanup only. It must not stop
  CAN front-end drain and must not reset or reconfigure MCP/CAN/transceiver
  state.

## Passive Proof Boundary

`CAPABILITY v6` hardware fields are claims and artifact references:

- silent strapped;
- galvanic isolated;
- power-off passive;
- reset safe;
- TXD gated;
- normal enable path not populated;
- field SKU ID;
- external analyzer artifact ID;
- hotplug pass count.

They are not proof by themselves. Vehicle-impact-free PASS requires external
analyzer/scope/DTC evidence and matching artifact IDs.

## Two-Bus Product Rule

The current product is two-bus RX only. One-bus passive artifacts are invalid,
but mismatch diagnostics must remain visible so wrong upload or wiring mistakes
are not hidden.

## Firmware Guard Rule

Passive builds must fail if they link or enable host downlink, host CAN TX,
control TX, test TX, USB reconnect reset, MCP normal mode, or built-in CAN
normal/ACK-capable mode.

