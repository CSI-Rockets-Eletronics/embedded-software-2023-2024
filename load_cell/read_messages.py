import sys
import requests
import time

FETCH_INTERVAL_SEC = 1

if len(sys.argv) < 4:
    print(
        "Usage: python read_messages.py [url] [environmentKey] [device]",
        file=sys.stderr,
    )
    sys.exit(1)

url = sys.argv[1]
environmentKey = sys.argv[2]
device = sys.argv[3]


def fetch_ts():
    result = requests.get(
        f"{url}/ts",
        headers={"content-type": "application/json"},
    )

    if result.status_code != 200:
        raise Exception(f"Error fetching ts from server (status: {result.status_code})")

    return result.json()


last_ts = fetch_ts()

while True:
    time.sleep(FETCH_INTERVAL_SEC)

    params = {
        "environmentKey": environmentKey,
        "device": device,
        "afterTs": last_ts,
    }

    try:
        result = requests.get(
            f"{url}/messages/next",
            params=params,
            headers={"content-type": "application/json"},
        )
    except Exception as e:
        if isinstance(e, KeyboardInterrupt):
            sys.exit(0)

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

    if result.text == "NONE":
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
