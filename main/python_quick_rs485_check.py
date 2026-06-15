#!/usr/bin/env python3
"""
Quick RS485(Modbus RTU) read check via USB-TTL/CH340.

Default request frame matches your known-good probe:
  addr=1, func=0x03, reg=0x0000, qty=1
  request hex: 01 03 00 00 00 01 84 0A

Usage examples:
  python3 main/python_quick_rs485_check.py
  python3 main/python_quick_rs485_check.py --port /dev/cu.usbserial-1130
  python3 main/python_quick_rs485_check.py --baud 9600 --tries 5 --timeout 0.5
"""

from __future__ import annotations

import argparse
import glob
import sys
import time
from typing import List, Optional, Tuple

try:
    import serial
    from serial.tools import list_ports
except Exception as exc:  # pragma: no cover
    print("[ERR] Missing dependency: pyserial")
    print("Install with: pip install pyserial")
    print(f"Detail: {exc}")
    sys.exit(2)


def modbus_crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def build_req(addr: int, func: int, reg: int, qty: int) -> bytes:
    body = bytes(
        [
            addr & 0xFF,
            func & 0xFF,
            (reg >> 8) & 0xFF,
            reg & 0xFF,
            (qty >> 8) & 0xFF,
            qty & 0xFF,
        ]
    )
    crc = modbus_crc16(body)
    return body + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


def parse_resp(resp: bytes, addr: int, func: int) -> Tuple[bool, str, Optional[int]]:
    if len(resp) < 5:
        return False, f"too short: {len(resp)}", None

    if resp[0] != (addr & 0xFF):
        return False, f"addr mismatch: got 0x{resp[0]:02X}", None

    # Modbus exception response
    if resp[1] == ((func & 0xFF) | 0x80):
        if len(resp) >= 5:
            return False, f"modbus exception: code=0x{resp[2]:02X}", None
        return False, "modbus exception (short frame)", None

    if resp[1] != (func & 0xFF):
        return False, f"func mismatch: got 0x{resp[1]:02X}", None

    byte_count = resp[2]
    expect_len = 3 + byte_count + 2
    if len(resp) < expect_len:
        return False, f"incomplete frame: got {len(resp)}, need {expect_len}", None

    frame = resp[:expect_len]
    crc_calc = modbus_crc16(frame[:-2])
    crc_recv = frame[-2] | (frame[-1] << 8)
    if crc_calc != crc_recv:
        return False, f"crc mismatch: calc=0x{crc_calc:04X}, recv=0x{crc_recv:04X}", None

    if byte_count < 2:
        return False, f"byte_count too small: {byte_count}", None

    value = (frame[3] << 8) | frame[4]
    return True, "ok", value


def hexs(b: bytes) -> str:
    return " ".join(f"{x:02X}" for x in b)


def detect_ports() -> List[str]:
    picked: List[str] = []

    # Prioritize CH340-like descriptions from pyserial enumeration.
    try:
        for p in list_ports.comports():
            text = f"{p.device} {p.description} {p.manufacturer} {p.hwid}".lower()
            if any(k in text for k in ["ch340", "ch34", "wch", "usb serial", "usb-serial"]):
                picked.append(p.device)
    except Exception:
        pass

    # Fallback by path patterns on macOS.
    if not picked:
        patterns = [
            "/dev/cu.usbserial*",
            "/dev/tty.usbserial*",
            "/dev/cu.wchusbserial*",
            "/dev/tty.wchusbserial*",
            "/dev/cu.*CH34*",
            "/dev/tty.*CH34*",
        ]
        for pat in patterns:
            picked.extend(glob.glob(pat))

    # De-duplicate while preserving order.
    seen = set()
    uniq = []
    for d in picked:
        if d not in seen:
            uniq.append(d)
            seen.add(d)
    return uniq


def read_once(
    port: str,
    baud: int,
    parity: str,
    stopbits: int,
    timeout: float,
    req: bytes,
) -> bytes:
    parity_map = {
        "N": serial.PARITY_NONE,
        "E": serial.PARITY_EVEN,
        "O": serial.PARITY_ODD,
    }
    stop_map = {
        1: serial.STOPBITS_ONE,
        2: serial.STOPBITS_TWO,
    }

    with serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=parity_map[parity],
        stopbits=stop_map[stopbits],
        timeout=timeout,
        write_timeout=timeout,
    ) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        ser.write(req)
        ser.flush()

        deadline = time.time() + timeout
        rx = bytearray()
        while time.time() < deadline and len(rx) < 256:
            n = ser.in_waiting
            if n > 0:
                rx.extend(ser.read(n))
            else:
                chunk = ser.read(1)
                if chunk:
                    rx.extend(chunk)
        return bytes(rx)


def main() -> int:
    parser = argparse.ArgumentParser(description="Quick Modbus RTU check over CH340/USB-TTL")
    parser.add_argument("--port", default=None, help="Serial port, e.g. /dev/cu.usbserial-1130")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--parity", choices=["N", "E", "O"], default="N")
    parser.add_argument("--stopbits", type=int, choices=[1, 2], default=1)
    parser.add_argument("--timeout", type=float, default=0.5)
    parser.add_argument("--tries", type=int, default=3)
    parser.add_argument("--addr", type=lambda x: int(x, 0), default=0x01)
    parser.add_argument("--func", type=lambda x: int(x, 0), default=0x03)
    parser.add_argument("--reg", type=lambda x: int(x, 0), default=0x0000)
    parser.add_argument("--qty", type=lambda x: int(x, 0), default=0x0001)
    args = parser.parse_args()

    req = build_req(args.addr, args.func, args.reg, args.qty)
    print(f"[INFO] request: {hexs(req)}")

    ports = [args.port] if args.port else detect_ports()
    if not ports:
        print("[ERR] no candidate serial port found. Use --port explicitly.")
        return 2

    print("[INFO] ports to test:")
    for p in ports:
        print(f"  - {p}")

    for p in ports:
        print(f"\n[TEST] {p} @ {args.baud} {args.parity}{args.stopbits}, timeout={args.timeout}s")
        for i in range(1, args.tries + 1):
            try:
                resp = read_once(
                    port=p,
                    baud=args.baud,
                    parity=args.parity,
                    stopbits=args.stopbits,
                    timeout=args.timeout,
                    req=req,
                )
            except Exception as exc:
                print(f"  try {i}: open/write/read error: {exc}")
                continue

            if not resp:
                print(f"  try {i}: timeout (0 bytes)")
                continue

            ok, reason, value = parse_resp(resp, args.addr, args.func)
            print(f"  try {i}: rx={hexs(resp)}")
            if ok:
                print(f"[HIT] {p} value={value} ({value / 10.0:.1f} C if x10 scale)")
                return 0
            print(f"  try {i}: parse_fail: {reason}")

    print("\n[MISS] no valid modbus response on tested ports")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
