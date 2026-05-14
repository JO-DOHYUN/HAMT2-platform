import argparse
import time

import serial

from send_host_can_tx_request import build_frame, parse_data
from verify_typed_stream import describe, parse_frame, u32


def is_matching_audit(frame: dict, can_id: int) -> bool:
    if frame.get("type") != 2:
        return False
    payload = frame.get("payload", b"")
    if len(payload) < 30:
        return False
    return (u32(payload, 8) & 0x1FFFFFFF) == can_id and payload[13] == 1


def main() -> None:
    parser = argparse.ArgumentParser(description="Send HOST_CAN_TX_REQUEST and watch CONTROL_ACK/CAN_TX_RAW.")
    parser.add_argument("--port", default="COM6")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--command-id", type=lambda s: int(s, 0), default=0x1001)
    parser.add_argument("--id", type=lambda s: int(s, 0), default=0x503)
    parser.add_argument("--data", default="00 00 00 00 00 00 00 00")
    parser.add_argument("--seconds", type=float, default=4.0)
    args = parser.parse_args()

    frame = build_frame(args.command_id, 1, args.id, parse_data(args.data))
    buf = bytearray()
    sent = False
    saw_ack = False
    saw_audit = False
    deadline = time.time() + args.seconds

    with serial.Serial(args.port, args.baud, timeout=0.05) as ser:
        ser.reset_input_buffer()
        while time.time() < deadline:
            if not sent and time.time() < deadline - max(0.5, args.seconds - 0.5):
                ser.write(frame)
                ser.flush()
                sent = True
                print(f"sent command=0x{args.command_id:08X} id=0x{args.id:X} bytes={len(frame)}")

            buf += ser.read(4096)
            while True:
                parsed = parse_frame(buf)
                if parsed is None:
                    break
                if parsed.get("type") == 6:
                    saw_ack = True
                    print(describe(parsed))
                elif is_matching_audit(parsed, args.id):
                    saw_audit = True
                    print(describe(parsed))
                elif parsed.get("type") == 7:
                    print(describe(parsed))

    print(f"result ack={int(saw_ack)} audit={int(saw_audit)}")
    raise SystemExit(0 if saw_ack and saw_audit else 2)


if __name__ == "__main__":
    main()
