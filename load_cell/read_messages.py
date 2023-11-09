import sys
import requests
import json

FETCH_INTERVAL_MS = 1000

if len(sys.argv) < 4:
    print("Usage: python read_messages.py [url] [environmentKey] [path]", file=sys.stderr)
    sys.exit(1)

url = sys.argv[1]
environmentKey = sys.argv[2]
path = sys.argv[3]

last_ts = None

while True:
    params = {
        "environmentKey": environmentKey,
        "path": path,
        "afterTs": last_ts,
    }

    try:
        result = requests.get(
            f"{url}/messages/next",
            params=params,
            headers={
                "content-type": "application/json",
            },
        )
    except:
        print(
            "Error reading messages from server (requests.post failed)",
            file=sys.stderr,
        )
        continue

    if result.status_code != 200:
        print(
            f"Error reading messages from server (status: {result.status_code})",
            file=sys.stderr,
        )
        continue

    body = result.json()
    if "ts" not in body:
        print("Messages response is missing ts field", file=sys.stderr)
        continue
    if "data" not in body:
        print("Messages response is missing data field", file=sys.stderr)
        continue

    last_ts = body["ts"]

    if body["data"] is not None:
        print(body["data"], flush=True)
        print("Received message:", body["data"], file=sys.stderr)
