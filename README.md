# HAMT2-platform

Monorepo for the HAMT2 CSM firmware, VMS/Qt application, and shared typed
transport/protocol contracts.

## Layout

- `board/`, `src/`, `include/`: CSM firmware and board-facing documentation
- `qt/`: VMS/Qt application work area
- `shared/`: cross-project protocol and record contracts
- `docs/`: architecture, AI harness, and verification notes
- `AGENTS.md`, `BRIEF.md`: Codex routing and current repository state

## Current CAN Direction

The Mid Carrier migration target is Classic CAN 2.0, not CAN FD:

- `bus0`: Mid Carrier J14 `CAN0_TX/RX` with ADA-5708 TJA1051T/3 transceiver
- `bus1`: Mid Carrier J4 CAN terminal through onboard transceiver path
- MCP2515/TJA1050 remains only for legacy bench and replay/import work
