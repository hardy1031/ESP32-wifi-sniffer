#!/usr/bin/env python3
"""bridge.py: forward the ESP32's PCAP byte stream from a serial port to stdout.

Usage:
    python tools/bridge.py --port /dev/cu.usbserial-1420 --baud 1152000 | wireshark -k -i -

What it does:
  1. Opens the serial port at the given baud rate (must match the ESP32).
  2. Resets the ESP32 (by default) so we catch the 24-byte PCAP file header
     it sends once at boot.
  3. Throws away boot text until it sees the PCAP magic number
     (0xA1B2C3D4, sent as D4 C3 B2 A1 by the little-endian ESP32).
  4. From the magic onward, copies every byte to stdout unchanged.

All status messages go to stderr, so they never mix into the binary stream.
"""

import argparse
import sys
import time

try:
    import serial  # pyserial
except ImportError:
    sys.exit("pyserial not found: pip install pyserial  (or run inside the ESP-IDF python env)")

MAGIC_LE = bytes.fromhex("d4c3b2a1")  # 0xA1B2C3D4 written by a little-endian CPU (ESP32)
MAGIC_BE = bytes.fromhex("a1b2c3d4")  # same magic written big-endian (just in case)


def log(msg: str) -> None:
    print(f"[bridge] {msg}", file=sys.stderr, flush=True)


def open_port(port: str, baud: int) -> serial.Serial:
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baud
    ser.timeout = 0.1
    # On ESP32 dev boards, RTS -> EN (reset) and DTR -> GPIO0 (boot mode).
    # Keep both released so opening the port doesn't hold the chip in reset/download mode.
    ser.dtr = False
    ser.rts = False
    ser.open()
    return ser


def reset_esp32(ser: serial.Serial) -> None:
    """Pulse EN low via RTS (normal boot, GPIO0 left high)."""
    ser.dtr = False
    ser.rts = True   # EN low -> chip held in reset
    time.sleep(0.1)
    ser.rts = False  # EN high -> chip boots normally
    ser.reset_input_buffer()


def wait_for_magic(ser: serial.Serial) -> bytes:
    """Discard bytes until the PCAP magic appears. Returns the magic + anything after it."""
    buf = b""
    skipped = 0
    while True:
        chunk = ser.read(4096)
        if not chunk:
            continue
        buf += chunk
        for magic in (MAGIC_LE, MAGIC_BE):
            i = buf.find(magic)
            if i != -1:
                skipped += i
                log(f"PCAP magic found ({'little' if magic == MAGIC_LE else 'big'}-endian), "
                    f"skipped {skipped} bytes of boot text")
                return buf[i:]
        # keep the last 3 bytes in case the magic is split across reads
        skipped += max(0, len(buf) - 3)
        buf = buf[-3:]


def main() -> None:
    ap = argparse.ArgumentParser(description="Serial -> stdout bridge for ESP32 PCAP stream")
    ap.add_argument("--port", required=True, help="e.g. /dev/cu.usbserial-1420")
    ap.add_argument("--baud", type=int, default=1152000, help="must match the ESP32 (default 1152000)")
    ap.add_argument("--no-reset", action="store_true", help="don't reset the ESP32 on start")
    args = ap.parse_args()

    if sys.stdout.isatty():
        log("warning: stdout is a terminal; pipe me into wireshark: ... | wireshark -k -i -")

    ser = open_port(args.port, args.baud)
    log(f"opened {args.port} @ {args.baud} baud")

    if not args.no_reset:
        reset_esp32(ser)
        log("ESP32 reset; waiting for PCAP header...")

    out = sys.stdout.buffer
    total = 0
    try:
        first = wait_for_magic(ser)
        out.write(first)
        out.flush()
        total += len(first)

        while True:
            chunk = ser.read(4096)
            if chunk:
                out.write(chunk)
                out.flush()          # send now, so Wireshark updates live
                total += len(chunk)
    except BrokenPipeError:
        log("wireshark closed the pipe; exiting")
    except KeyboardInterrupt:
        log("stopped by user")
    finally:
        log(f"forwarded {total} bytes")
        ser.close()


if __name__ == "__main__":
    main()
