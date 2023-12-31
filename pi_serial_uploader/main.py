import json
import time
import requests
import sys
import serial

MAX_RECORDS_PER_BATCH = 10
MAX_LINE_LENGTH = 64

URL = "http://localhost:3000"
ENVIRONMENT_KEY = "0"
DEVICE = "Scientific"

ser = serial.Serial("/dev/ttyS0", 115200)

# throw away possibly partial line
ser.readline()

while True:
    records = []

    while ser.in_waiting > MAX_LINE_LENGTH:
        input_line = ser.readline()

        if len(records) >= MAX_RECORDS_PER_BATCH:
            continue

        records.append(
            {
                "ts": time.time_ns() // 1000,
                "data": json.loads(input_line),
            }
        )

    if len(records) == 0:
        continue

    body = {
        "environmentKey": ENVIRONMENT_KEY,
        "device": DEVICE,
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
