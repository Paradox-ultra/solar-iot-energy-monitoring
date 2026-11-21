#!/usr/bin/env python3
"""USB serial CSV logger for ESP32 solar monitor firmware."""
from __future__ import annotations

import argparse
import csv
import logging
import signal
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None

FIELDS = ["timestamp_utc", "voltage_V", "current_mA", "power_mW",
          "energy_mWh", "temperature_C", "adc0"]
SENTINELS = {-999.0, -1.0}


def discover_ports() -> list[str]:
    if list_ports is None:
        raise RuntimeError("pyserial missing; install requirements.txt")
    return [p.device for p in list_ports.comports()]


def parse_row(line: str) -> Optional[dict[str, str]]:
    parts = line.strip().split(",")
    if not parts or parts[0] != "CSV_ROW":
        return None
    if len(parts) != 7:
        raise ValueError(f"expected 7 fields including CSV_ROW, got {len(parts)}")
    row = {"timestamp_utc": datetime.now(timezone.utc).isoformat(timespec="milliseconds")}
    for name, raw in zip(FIELDS[1:], parts[1:]):
        value = float(raw)
        row[name] = "" if value in SENTINELS else raw
    return row


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--port", help="Serial port, e.g. COM5 or /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--output", type=Path, default=Path("solar_realtime_data.csv"))
    ap.add_argument("--timeout", type=float, default=2.5)
    args = ap.parse_args()

    if serial is None:
        ap.error("pyserial missing; run python -m pip install -r requirements.txt")
    ports = discover_ports()
    if not args.port:
        if not ports:
            ap.error("No serial ports detected; connect ESP32 or pass --port.")
        if len(ports) > 1:
            print("Available ports:", ", ".join(ports), file=sys.stderr)
            ap.error("Multiple ports detected; specify --port.")
        args.port = ports[0]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    new_file = not args.output.exists() or args.output.stat().st_size == 0
    stop = False

    def request_stop(_signum, _frame):
        nonlocal stop
        stop = True

    signal.signal(signal.SIGINT, request_stop)
    if hasattr(signal, "SIGTERM"):
        signal.signal(signal.SIGTERM, request_stop)

    logging.info("Opening %s at %d baud; output=%s", args.port, args.baud, args.output)
    try:
        with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser, args.output.open("a", newline="", encoding="utf-8") as fp:
            writer = csv.DictWriter(fp, fieldnames=FIELDS)
            if new_file:
                writer.writeheader()
                fp.flush()
            while not stop:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                if not line.startswith("CSV_ROW,"):
                    print(line)
                    continue
                try:
                    row = parse_row(line)
                except (ValueError, OverflowError) as exc:
                    logging.warning("Discarding malformed row: %s", exc)
                    continue
                if row is not None:
                    writer.writerow(row)
                    fp.flush()
                    print("Logged telemetry row")

    except serial.SerialException as exc:
        logging.error("Serial connection failed: %s", exc)
        return 2
    except OSError as exc:
        logging.error("Output/port error: %s", exc)
        return 3
    logging.info("Logger stopped")
    return 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    raise SystemExit(main())
