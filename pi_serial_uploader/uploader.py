import json
import time
import requests
import sys
import serial
from typing import Callable, Any
import threading

MAX_RECORDS_PER_BATCH = 50

URL = "http://localhost:3000"
ENVIRONMENT_KEY = "0"


def post_records(device: str, records: list[Any]):
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
        return

    if result.status_code != 200:
        print(
            f"Error sending records to server (status: {result.status_code})",
            file=sys.stderr,
        )
    else:
        print(
            f"Sent {len(records)} records to server for device {device}",
            file=sys.stderr,
        )


def run(
    parse_device: str | Callable[[bytes], str],
    delimiter: bytes = b"\n",
    parse_packet: Callable[[bytes], str] = lambda x: x.decode("utf-8"),
):
    """
    Continuously read from serial port and send data to server.

    parse_device: device name to send to server, or function that takes a serial packet and returns a device name (and throws an exception if it fails)
    delimiter: delimiter between serial packets
    parse_packet: function to parse a serial packet into json (and throws an exception if it fails)
    """

    ser = serial.Serial("/dev/ttyS0", 115200)

    # throw away possibly partial packet
    ser.read_until(delimiter)

    while True:
        # device -> records
        records_dict: dict[str, list[Any]] = {}
        records_count = 0

        while ser.in_waiting > 0:
            input_packet = ser.read_until(delimiter)
            # remove delimiter
            input_packet = input_packet[: -len(delimiter)]

            try:
                device = (
                    parse_device(input_packet)
                    if callable(parse_device)
                    else parse_device
                )
                input_parsed = parse_packet(input_packet)
            except Exception as e:
                print(
                    "Error parsing input packet:",
                    e,
                    file=sys.stderr,
                )
                continue

            if records_count >= MAX_RECORDS_PER_BATCH:
                continue

            try:
                data = json.loads(input_parsed)
            except json.JSONDecodeError:
                print(
                    f"Error parsing input json: {input_parsed}",
                    file=sys.stderr,
                )
                continue

            if device not in records_dict:
                records_dict[device] = []

            records_dict[device].append(
                {
                    "ts": time.time_ns() // 1000,
                    "data": data,
                }
            )
            records_count += 1

        for device, records in records_dict.items():
            # Perform the network request on a separate thread
            threading.Thread(target=post_records, args=(device, records)).start()
