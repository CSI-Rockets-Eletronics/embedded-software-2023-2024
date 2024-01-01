import json
import time
import requests
import sys
import serial


def run(device: str):
    MAX_RECORDS_PER_BATCH = 50

    URL = "http://localhost:3000"
    ENVIRONMENT_KEY = "0"

    ser = serial.Serial("/dev/ttyS0", 115200)

    # throw away possibly partial line
    ser.readline()

    while True:
        records = []

        while ser.in_waiting > 0:
            input_line = ser.readline()

            if len(records) >= MAX_RECORDS_PER_BATCH:
                continue

            try:
                data = json.loads(input_line)
            except json.JSONDecodeError:
                print(
                    f"Error parsing input line: {input_line}",
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
