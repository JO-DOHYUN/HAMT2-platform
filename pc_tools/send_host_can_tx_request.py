import argparse
import struct
import time

import serial


SOF = b"\xA5\x5A"
VERSION = 1
HOST_CAN_TX_REQUEST = 10


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def parse_data(text: str) -> bytes:
    cleaned = text.replace(",", " ").replace(":", " ").replace("-", " ")
    parts = [p for p in cleaned.split() if p]
    data = bytes(int(p, 16) & 0xFF for p in parts)
    if len(data) > 8:
        raise ValueError("CAN data must be at most 8 bytes")
    return data


def build_frame(command_id: int, bus: int, can_id: int, data: bytes, extended: bool = False) -> bytes:
    dlc = min(len(data), 8)
    padded = data[:8] + bytes(max(0, 8 - len(data)))
    payload = struct.pack("<IBBIB", command_id, bus, 0x01 if extended else 0x00, can_id, dlc) + padded
    header_payload = struct.pack("<BBBHH", VERSION, HOST_CAN_TX_REQUEST, 0, command_id & 0xFFFF, len(payload)) + payload
    return SOF + header_payload + struct.pack("<H", crc16_ccitt(header_payload))


def main() -> None:
    parser = argparse.ArgumentParser(description="Send one HOST_CAN_TX_REQUEST typed frame to the board.")
    parser.add_argument("--port", default="COM6")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--command-id", type=lambda s: int(s, 0), default=1)
    parser.add_argument("--bus", type=int, default=1)
    parser.add_argument("--id", type=lambda s: int(s, 0), default=0x503)
    parser.add_argument("--data", default="00 00 00 00 00 00 00 00")
    parser.add_argument("--extended", action="store_true")
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--period-ms", type=float, default=50.0)
    args = parser.parse_args()

    data = parse_data(args.data)
    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        for index in range(args.repeat):
            command_id = (args.command_id + index) & 0xFFFFFFFF
            frame = build_frame(command_id, args.bus, args.id, data, args.extended)
            ser.write(frame)
            ser.flush()
            print(f"sent command=0x{command_id:08X} bus={args.bus} id=0x{args.id:X} dlc=8 bytes={len(frame)}")
            if index + 1 < args.repeat:
                time.sleep(args.period_ms / 1000.0)


if __name__ == "__main__":
    main()
