import json
import time
import requests
import sys
import serial

MAX_RECORDS_PER_BATCH = 10
MAX_LINE_LENGTH = 64

url = "http://localhost:3000"
environmentKey = "0"
device = "Scientific"

ser = serial.Serial("/dev/ttyS0", 115200)

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
        "environmentKey": environmentKey,
        "device": device,
        "records": records,
    }

    try:
        result = requests.post(
            f"{url}/records/batch",
            # no compression
            json=body,
            headers={"content-type": "application/json"},
        )
    except:
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
