import sys
import requests
import gzip
import json
import select

if len(sys.argv) < 4:
    print("Usage: python write_records.py [url] [stationId] [source]", file=sys.stderr)
    sys.exit(1)

url = sys.argv[1]
stationId = sys.argv[2]
source = sys.argv[3]


def has_input():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])


while True:
    records = []
    seen_timestamps = set()

    while has_input():
        input_line = input()
        split = input_line.split(" ", 2)
        if len(split) != 3 or split[0] != "rec:":
            continue
        _, timestamp, data = split

        timestamp = int(timestamp)
        data = json.loads(data)

        if timestamp in seen_timestamps:
            continue

        records.append(
            {
                "timestamp": timestamp,
                "data": data,
            }
        )

    if len(records) == 0:
        continue

    body = {
        "stationId": stationId,
        "source": source,
        "records": records,
    }

    body_compressed = gzip.compress(json.dumps(body).encode("utf-8"))

    try:
        result = requests.post(
            f"{url}/record/batch",
            data=body_compressed,
            headers={
                "content-type": "application/json",
                "content-encoding": "gzip",
            },
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
            f"Sent {len(records)} records to server (length={len(body_compressed)})",
            file=sys.stderr,
        )
