import sys
import requests
import gzip
import json
import select

if len(sys.argv) < 4:
    print(
        "Usage: python write_records.py [url] [environmentKey] [path]", file=sys.stderr
    )
    sys.exit(1)

url = sys.argv[1]
environmentKey = sys.argv[2]
path = sys.argv[3]


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
        _, ts, data = split

        ts = int(ts)
        data = json.loads(data)

        if ts in seen_timestamps:
            continue

        records.append(
            {
                "ts": ts,
                "data": data,
            }
        )

    if len(records) == 0:
        continue

    body = {
        "environmentKey": environmentKey,
        "path": path,
        "records": records,
    }

    body_compressed = gzip.compress(json.dumps(body).encode("utf-8"))

    try:
        result = requests.post(
            f"{url}/records/batch",
            data=body_compressed,
            headers={"content-type": "application/json-gzip"},
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
