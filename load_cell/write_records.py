import sys
import requests
import gzip
import json
import select

MAX_RECORDS_PER_BATCH = 10

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


while True:
    records = []

    while has_input():
        input_line = input()

        if len(records) >= MAX_RECORDS_PER_BATCH:
            continue

        split = input_line.split(" ")
        if len(split) != 4 or split[0] != "rec:":
            continue
        _, ts, lbs1, lbs2 = split

        records.append(
            {
                "ts": int(ts),
                "lbs1": json.loads(lbs1),
                "lbs2": json.loads(lbs2),
            }
        )

    if len(records) == 0:
        continue

    body = {
        "environmentKey": environmentKey,
        "device": device,
        "records": records,
    }

    # body_compressed = gzip.compress(json.dumps(body).encode("utf-8"))

    try:
        result = requests.post(
            f"{url}/records/batch",
            # no compression
            json=body,
            headers={"content-type": "application/json"},
            # compression (not needed for localhost, as we'd just be wasting CPU)
            # data=body_compressed,
            # headers={"content-type": "application/json-gzip"},
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
            # f"Sent {len(records)} records to server (compressed length={len(body_compressed)})",
            f"Sent {len(records)} records to server",
            file=sys.stderr,
        )
