import json
import time
import requests
import sys
import serial
from typing import Callable


def run(
    device: str,
    serial_delimiter: bytes = "\n",
    parse_serial_packet: Callable[[bytes], str] = lambda x: x.decode("utf-8"),
):
    MAX_RECORDS_PER_BATCH = 50

    URL = "http://localhost:3000"
    ENVIRONMENT_KEY = "0"

    ser = serial.Serial("/dev/ttyS0", 115200)

    # throw away possibly partial line
    ser.readline()

    while True:
        records = []

        while ser.in_waiting > 0:
            input_packet = ser.read_until(serial_delimiter)
            input_parsed = parse_serial_packet(input_packet)

            if len(records) >= MAX_RECORDS_PER_BATCH:
                continue

            try:
                data = json.loads(input_parsed)
            except json.JSONDecodeError:
                print(
                    f"Error parsing input json: {input_parsed}",
                    file=sys.stderr,
                )
                continue

            records.append(
                {
                    "ts": time.time_ns() // 1000,
                    "data": data,
                }
            )

        if len(records) == 0:
            continue

        body = {
            "environmentKey": ENVIRONMENT_KEY,
            "device": device,
            "records": records,
        }

        try:
            result = requests.post(
                f"{URL}/records/batch",
                # no compression
                json=body,
                headers={"content-type": "application/json"},
            )
        except Exception as e:
            if isinstance(e, KeyboardInterrupt):
                sys.exit(0)

            print(
                f"Error sending records to server (requests.post failed)",
                file=sys.stderr,
            )
            continue

        if result.status_code != 200:
            print(
                f"Error sending records to server (status: {result.status_code})",
                file=sys.stderr,
            )
        else:
            print(
                f"Sent {len(records)} records to server",
                file=sys.stderr,
            )
