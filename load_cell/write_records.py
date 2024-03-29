import sys
import requests
import select
import time

DEBUG_PRINT = False
MAX_RECORDS_PER_BATCH = 500

if len(sys.argv) < 4:
    print(
        "Usage: python write_records.py [url] [environmentKey] [device]",
        file=sys.stderr,
    )
    sys.exit(1)

url = sys.argv[1]
environmentKey = sys.argv[2]
device = sys.argv[3]


def has_input():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])


record_count = 0
start_time = time.time()

while True:
    records = []

    while has_input():
        input_line = input()

        if len(records) >= MAX_RECORDS_PER_BATCH:
            continue

        split = input_line.split(" ")
        if len(split) != 3 or split[0] != "rec:":
            continue
        _, ts, lbs = split

        records.append(
            {
                "ts": int(ts),
                "data": float(lbs),
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
    elif DEBUG_PRINT:
        print(
            # f"Sent {len(records)} records to server (compressed length={len(body_compressed)})",
            f"Sent {len(records)} records to server",
            file=sys.stderr,
        )

    record_count += len(records)

    elapsed_time = time.time() - start_time
    if elapsed_time >= 1:
        frequency = record_count / elapsed_time
        print(f"Frequency: {frequency} records/second", file=sys.stderr)
        record_count = 0
        start_time = time.time()
