import sys
import requests
import json

FETCH_INTERVAL_MS = 1000

if len(sys.argv) < 4:
    print("Usage: python read_messages.py [url] [stationId] [target]", file=sys.stderr)
    sys.exit(1)

url = sys.argv[1]
stationId = sys.argv[2]
target = sys.argv[3]

while True:
    body = {
        "stationId": stationId,
        "target": target,
    }

    try:
        result = requests.post(
            f"{url}/message/next",
            json=body,
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
    if "data" not in body:
        print("Messages response is missing data field", file=sys.stderr)
        continue

    if body["data"] is not None:
        print(body["data"], flush=True)
        print("Received message:", body["data"], file=sys.stderr)
