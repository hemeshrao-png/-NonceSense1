#!/usr/bin/env python3
"""host_test.py - host-side test for NonceSense Task 1.
Usage:  python3 host_test.py /dev/ttyUSB0        (Windows: COM5)
Needs:  pip install pyserial
"""
import random
import sys
import zlib

KEY = 0xA5C3E1F7          # must match SECRET_KEY in main.c


def expected(challenge: int) -> int:
    data = challenge.to_bytes(4, "big") + KEY.to_bytes(4, "big")
    return zlib.crc32(data) & 0xFFFFFFFF


def main() -> int:
    import serial  # imported late so the file can be read without pyserial
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
    ser = serial.Serial(port, 115200, timeout=1)
    fails = 0

    for _ in range(10):
        ch = random.getrandbits(32)
        ser.write(f"AUTH:{ch:08X}\n".encode())
        got = ser.readline().decode().strip()
        want = f"RESP:{expected(ch):08X}"
        ok = got == want
        fails += not ok
        print(f"{'PASS' if ok else 'FAIL'}  sent AUTH:{ch:08X}  got {got!r}  want {want!r}")

    for bad in ("AUTH:XYZ", "AUTH:12345678Z", "HELLO", "AUTH:GGGGGGGG"):
        ser.write(f"{bad}\n".encode())
        got = ser.readline().decode().strip()
        ok = got == "ERR"
        fails += not ok
        print(f"{'PASS' if ok else 'FAIL'}  sent {bad!r}  got {got!r}  want 'ERR'")

    print("ALL PASSED" if fails == 0 else f"{fails} FAILED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
