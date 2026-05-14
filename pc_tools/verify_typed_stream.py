import argparse
import struct
import time

import serial


SOF = b"\xA5\x5A"
TYPE_NAMES = {
    1: "CAN_RX_RAW",
    2: "CAN_TX_RAW",
    3: "ENC_EDGE_RAW",
    4: "ENC_DERIVED",
    5: "ADC_SAMPLE",
    6: "CONTROL_ACK",
    7: "BOARD_EVENT",
    8: "BOARD_HEALTH",
    9: "CAPABILITY",
}


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


def u16(payload: bytes, offset: int) -> int:
    return struct.unpack_from("<H", payload, offset)[0]


def u32(payload: bytes, offset: int) -> int:
    return struct.unpack_from("<I", payload, offset)[0]


def u64(payload: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", payload, offset)[0]


def i32(payload: bytes, offset: int) -> int:
    return struct.unpack_from("<i", payload, offset)[0]


def i64(payload: bytes, offset: int) -> int:
    return struct.unpack_from("<q", payload, offset)[0]


def parse_frame(buf: bytearray):
    start = buf.find(SOF)
    if start < 0:
        del buf[:-1]
        return None
    if start > 0:
        del buf[:start]
    if len(buf) < 11:
        return None

    version = buf[2]
    rtype = buf[3]
    flags = buf[4]
    seq = u16(buf, 5)
    length = u16(buf, 7)
    frame_len = 2 + 1 + 1 + 1 + 2 + 2 + length + 2
    if length > 256:
        del buf[:2]
        return None
    if len(buf) < frame_len:
        return None

    frame = bytes(buf[:frame_len])
    del buf[:frame_len]
    expected = u16(frame, frame_len - 2)
    actual = crc16_ccitt(frame[2:-2])
    if expected != actual:
        return {"bad_crc": True, "expected": expected, "actual": actual}
    return {
        "version": version,
        "type": rtype,
        "flags": flags,
        "seq": seq,
        "payload": frame[9:-2],
    }


def describe(frame):
    if frame.get("bad_crc"):
        return f"[BAD_CRC] expected=0x{frame['expected']:04X} actual=0x{frame['actual']:04X}"

    rtype = frame["type"]
    payload = frame["payload"]
    name = TYPE_NAMES.get(rtype, f"type_{rtype}")
    seq = frame["seq"]

    if rtype in (1, 2) and len(payload) >= 30:
        mono = u64(payload, 0)
        can_id_flags = u32(payload, 8)
        can_id = can_id_flags & 0x1FFFFFFF
        ext = (can_id_flags >> 29) & 1
        dlc = payload[12] & 0x0F
        bus = payload[13]
        data = payload[14:22]
        total = u32(payload, 22)
        fail_or_drop = u32(payload, 26)
        tail = (
            f"rx_total={total} dropped={fail_or_drop}"
            if rtype == 1
            else f"tx_total={total} failed={fail_or_drop}"
        )
        return (
            f"[{name}] seq={seq} mono_us={mono} bus={bus} id=0x{can_id:X} ext={ext} "
            f"dlc={dlc} data={data.hex(' ')} {tail}"
        )

    if rtype == 3 and len(payload) >= 28:
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} pos={i64(payload, 8)} "
            f"tim3={u16(payload, 16)} ab={payload[18]} flags=0x{payload[19]:02X} "
            f"fault=0x{u32(payload, 20):08X} z_count={u32(payload, 24)}"
        )

    if rtype == 4 and len(payload) >= 28:
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} pos={i64(payload, 8)} "
            f"cps={i32(payload, 16)} tim3={u16(payload, 20)} ab={payload[22]} "
            f"fault=0x{u32(payload, 24):08X}"
        )

    if rtype == 5 and len(payload) >= 44:
        source = payload[16]
        count = min(payload[17], 8)
        bits = payload[18]
        flags = payload[19]
        channels = payload[20 : 20 + count]
        samples = [u16(payload, 28 + i * 2) for i in range(count)]
        pairs = " ".join(f"ch{channels[i]}={samples[i]}" for i in range(count))
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} sample_total={u32(payload, 8)} "
            f"dropped={u32(payload, 12)} source={source} bits={bits} "
            f"flags=0x{flags:02X} {pairs}"
        )

    if rtype == 6 and len(payload) >= 28:
        status = payload[12]
        reason = payload[13]
        bus = payload[14]
        dlc = payload[15] & 0x0F
        can_id_flags = u32(payload, 16)
        can_id = can_id_flags & 0x1FFFFFFF
        ext = (can_id_flags >> 29) & 1
        rtr = (can_id_flags >> 30) & 1
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} command=0x{u32(payload, 8):08X} "
            f"status={status} reason={reason} bus={bus} id=0x{can_id:X} ext={ext} rtr={rtr} "
            f"dlc={dlc} counter={u32(payload, 20)} rejected={u32(payload, 24)}"
        )

    if rtype == 7 and len(payload) >= 16:
        code = u16(payload, 8)
        if code == 10:
            detail = u16(payload, 10)
            counter = u32(payload, 12)
            stage = (detail >> 8) & 0xFF
            canstat = detail & 0xFF
            canctrl = (counter >> 24) & 0xFF
            canintf = (counter >> 16) & 0xFF
            eflg = (counter >> 8) & 0xFF
            extra = counter & 0xFF
            return (
                f"[{name}] seq={seq} mono_us={u64(payload, 0)} code={code} "
                f"stage={stage} CANSTAT=0x{canstat:02X} CANCTRL=0x{canctrl:02X} "
                f"CANINTF=0x{canintf:02X} EFLG=0x{eflg:02X} extra=0x{extra:02X}"
            )
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} code={u16(payload, 8)} "
            f"detail={u16(payload, 10)} counter={u32(payload, 12)}"
        )

    if rtype == 8 and len(payload) >= 52:
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} can_rx={u32(payload, 8)} "
            f"can_drop={u32(payload, 12)} tx_records={u32(payload, 20)} "
            f"queue={u32(payload, 24)} enc_faults={u32(payload, 28)} "
            f"enc_wrap={u32(payload, 32)} pos={i64(payload, 36)} "
            f"safety={payload[44]} inputs=0x{payload[45]:02X} "
            f"timer_ok={payload[46]} flags=0x{payload[47]:02X} "
            f"can_ok={(payload[47] >> 1) & 1} builtin_can_tx_ok={(payload[47] >> 2) & 1} "
            f"mcp_int_level={(payload[47] >> 3) & 1} mcp_exti_hint={(payload[47] >> 4) & 1} "
            f"voltage_adc_ok={(payload[47] >> 5) & 1} "
            f"fault=0x{u32(payload, 48):08X}"
        )

    if rtype == 9 and len(payload) >= 36:
        return (
            f"[{name}] seq={seq} mono_us={u64(payload, 0)} proto={payload[8]} "
            f"profile={payload[9]}.{payload[10]} mono_unit={payload[11]} "
            f"can_q={u32(payload, 12)} encoder_ppr={u32(payload, 16)} "
            f"encoder_freq_limit={u32(payload, 20)} adc_sample={payload[28]} "
            f"adc_channels={payload[31]} adc_bits={payload[32]} adc_period_ms={payload[33]}"
        )

    return f"[{name}] seq={seq} len={len(payload)} payload={payload.hex(' ')}"


def main():
    parser = argparse.ArgumentParser(description="Decode Portenta typed record stream.")
    parser.add_argument("--port", default="COM6")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--raw", action="store_true", help="Print every record, including high-rate CAN.")
    parser.add_argument("--seconds", type=float, default=0.0, help="Stop after this many seconds; 0 runs forever.")
    parser.add_argument("--max-records", type=int, default=0, help="Stop after this many printed records; 0 runs forever.")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    ser.reset_input_buffer()

    buf = bytearray()
    last_can_print = 0.0
    printed = 0
    deadline = time.time() + args.seconds if args.seconds > 0 else None
    while True:
        if deadline is not None and time.time() >= deadline:
            break
        buf += ser.read(4096)
        while True:
            frame = parse_frame(buf)
            if frame is None:
                break
            if frame.get("type") == 1 and not args.raw:
                now = time.time()
                if now - last_can_print < 0.5:
                    continue
                last_can_print = now
            print(describe(frame))
            printed += 1
            if args.max_records > 0 and printed >= args.max_records:
                return


if __name__ == "__main__":
    main()
