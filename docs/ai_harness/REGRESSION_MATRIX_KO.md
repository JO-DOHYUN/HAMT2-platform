# REGRESSION_MATRIX_KO

## CSM Regression - common
- typed frame SOF/length/CRC resync
- `CAPABILITY` record after boot
- `BOARD_HEALTH` periodic record
- host type 10 accepted ids -> `CONTROL_ACK` then `CAN_TX_RAW`
- rejected host commands -> `CONTROL_ACK status=0` and visible reason
- ADC raw -> `ADC_SAMPLE`, not fake CAN

## CSM Regression - legacy_mcp
- MCP RX -> `CAN_RX_RAW bus=0`
- built-in CAN RX -> `CAN_RX_RAW bus=1`
- MCP INT level hint + bounded polling, no falling-edge-only gate

## CSM Regression - mid_dual_can
- `CAPABILITY` profile major 2 exposes two bus descriptors
- bus0 descriptor: Mid J14 CAN0 + ADA-5708/TJA1051, monitor/system role
- bus1 descriptor: Mid J4 terminal CAN1 through onboard U2, drive/control role
- MCP2515 dependency is absent from the production build path
- ArduinoCore single-CAN limitation is reported until an internal CAN0 backend exists

## VMS Regression
- serial open does not imply board alive
- typed frame parser rejects bad CRC and resyncs
- exact typed bytes are stored append-only
- replay preserves source order and record type
- `CONTROL_ACK` and `CAN_TX_RAW` display as separate events
- legacy 20-byte import remains offline compatibility only

## Hardware Test Regression
Before repeating a failed hardware test, state:
- confirmed facts
- unconfirmed hypothesis
- what changed since the previous failed run
- expected evidence that would prove or disprove the hypothesis
