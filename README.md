# HAMT2-platform

Monorepo for the HAMT2 CSM firmware, VMS/Qt application, and shared typed
transport/protocol contracts.

This baseline freezes the working CSM state before VMS/Qt integration. The
repository layout is intentionally kept stable; VMS cleanup and integration are
next-step work.

## Layout

- `board/`, `src/`, `include/`: CSM firmware and board-facing documentation
- `qt/`: VMS/Qt application work area
- `shared/`: cross-project protocol and record contracts
- `docs/`: architecture, AI harness, and verification notes
- `AGENTS.md`, `BRIEF.md`: Codex routing and current repository state

## Current CAN Direction

The current Mid Carrier CSM target is Classic CAN 2.0, not CAN FD:

- `bus0`: external MCP2515/TJA1050 on Mid Carrier D7..D11 SPI pins
- `bus0` emits typed `CAN_RX_RAW` and audited `CAN_TX_RAW`
- `bus1`: Mid Carrier J4 CAN1 through onboard U2, enabled by the J4 SW2 pair
- `HOST_CAN_TX_REQUEST` is accepted on `bus0` and `bus1` for the allowlisted control IDs
- final dual-channel env: `portenta_h7_m7_mid_mcp2515_j4_dual_csm`
- Dual internal CAN0/CAN1 through TJA1051-class transceivers is deferred until a
  second internal CAN backend is implemented and HIL-proven

## Shared Contract

`shared/docs/TRANSPORT_AND_RECORDS_KO.md` is the source of truth for the CSM/VMS
typed stream, record payloads, bus capability descriptors, and host control
audit rules.
