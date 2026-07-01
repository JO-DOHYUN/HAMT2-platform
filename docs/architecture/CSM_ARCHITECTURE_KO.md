# CSM Architecture

## Product Identity

CSM is not a USB-CAN bridge in the product path. CSM Passive Product is a
two-bus RX-only passive CAN front-end that emits typed evidence over CDC when a
host session is present. Active TX/control belongs only to explicit bench/lab
profiles.

## Product Modes

| Mode | Purpose | Vehicle passive acceptance | CAN TX/ACK |
| --- | --- | --- | --- |
| Passive Product | two-bus field monitor/logger evidence | allowed only after hardware evidence | disabled / no ACK |
| Full Instrumented | bench/HIL control and diagnostics | forbidden | allowed by build flags |
| Bench ACK/TX Test | Kvaser/PCAN single-node TX counterpart | forbidden | ACK/TX allowed by explicit lab profile |

## Passive Product Data Flow

```text
Power/reset
  -> hardware-default safe pins
  -> CAN front-end passive init
  -> host absent drain-and-discard
  -> CDC session open
  -> uplink/session quarantine cleanup
  -> capability + lifecycle summaries
  -> new CAN_RX_SEGMENT evidence only
```

The CAN front-end drain must continue regardless of CDC host state. USB
open/close may clean only CDC/uplink/session payload state. It must not switch
MCP mode, reset transceiver/controller, enable TX gates, or replay old payloads.

## Module Ownership Target

- `protocol/TypedFrame`: SOF, header, seq, length, CRC, endian helpers.
- `protocol/TypedRecords`: shared record IDs and payload layout.
- `CapabilityPublisher`: firmware profile, bus descriptors, passive claims.
- `CanRxLane_MCP2515`: bus0 MCP2515 listen-only RX and passive readback.
- `CanRxLane_BuiltinSilent`: bus1 Mid Carrier J4/U2 silent monitor RX.
- `HostSessionRuntime`: CDC session epoch, DTR/session events, quarantine.
- `UplinkScheduler`: priority/descriptor/payload ownership, no silent clear.
- `HealthMonitor`: counters, violation latch, board health payload.
- `PassiveSupervisor`: mode readback, TXREQ zero verification, reset/power
  suspected latch.
- `ControlLane` / `CanTxAuditLane`: bench/full profile only.

`src/main.cpp` remains orchestration until the migration is finished, but new
behavior must be expressed through these ownership boundaries.

## Passive Invariants

- Passive Product compiles out host downlink, host TX, control TX, periodic test
  TX, and USB reconnect reset.
- bus0 MCP2515 starts and remains listen-only.
- bus1 built-in CAN starts and remains silent monitor RX.
- TXREQ bits must remain zero; violation latches and reports.
- Hardware evidence fields in CAPABILITY are claims/references, not proof.
- `passive_acceptance_allowed=true` requires nonzero hardware safety,
  bench verification, external analyzer artifact, and hotplug count.
- One-bus product artifacts are invalid; missing-bus mismatch diagnostics remain.
- Passive Product does not ACK. Kvaser/PCAN TX tests require another active node
  or a lab ACK/TX profile.

## Wire Contract

Typed transport v1 is the production live output. Legacy/diagnostic binaries are
not field product paths. Any wire-format change must update:

- `include/protocol/TypedRecords.h`
- `shared/docs/TRANSPORT_AND_RECORDS_KO.md`
- matching VSM typed parser and capability decoder
