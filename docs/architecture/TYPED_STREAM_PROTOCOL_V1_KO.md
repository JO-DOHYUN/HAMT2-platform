# TYPED_STREAM_PROTOCOL_V1_KO

## Canonical Contract
이 문서는 설명용이다. 실제 binary wire contract의 단일 기준은
`shared/docs/TRANSPORT_AND_RECORDS_KO.md`다.

## Transport
- SOF: `0xA5 0x5A`
- header: `version`, `record_type`, `flags`, `seq u16_le`, `payload_len u16_le`
- payload: record-specific little-endian bytes
- trailer: `crc16_ccitt_le`
- recovery: SOF scan, length check, CRC check, dispatch

## Live Records
- `1 CAN_RX_RAW`
- `2 CAN_TX_RAW`
- `3 ENC_EDGE_RAW`
- `4 ENC_DERIVED`
- `5 ADC_SAMPLE`
- `6 CONTROL_ACK`
- `7 BOARD_EVENT`
- `8 BOARD_HEALTH`
- `9 CAPABILITY`
- `10 HOST_CAN_TX_REQUEST` host-to-board only

## Compatibility
Legacy fixed 20-byte data is replay/import compatibility only. It must not be
mixed into the live typed stream parser.

