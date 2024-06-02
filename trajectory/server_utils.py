from typing import TypedDict, Optional
import requests

url = "http://localhost:3000"
environment_key = "0"

mpu_device = "MPU"
trajectory_device = "Trajectory"

calibrate_trajectory_message = "CALIBRATE"


class MpuRecordData(TypedDict):
    ts: int
    ax: int
    ay: int
    az: int
    gx: int
    gy: int
    gz: int


class MpuRecord(TypedDict):
    ts: int
    data: MpuRecordData


class TrajectoryRecordData(TypedDict):
    ts: int
    x: float
    y: float
    z: float
    vx: float
    vy: float
    vz: float
    ax: float
    ay: float
    az: float


def fetch_ts() -> int:
    """Retries forever"""
    while True:
        try:
            result = requests.get(
                f"{url}/ts",
                headers={"content-type": "application/json"},
            )
        except KeyboardInterrupt:
            raise
        except Exception as e:
            print(f"Error fetching ts: {e}")
            continue
        if result.status_code != 200:
            print(f"Error fetching ts: status {result.status_code}")
            continue
        # success!
        return result.json()


init_ts = fetch_ts()

last_mpu_record_ts = init_ts


def fetch_new_mpu_records(take: Optional[int] = None) -> list[MpuRecordData]:
    """Fetches at most `take` records. Returns an empty list on error"""
    global last_mpu_record_ts

    params = {
        "environmentKey": environment_key,
        "device": mpu_device,
        "startTs": last_mpu_record_ts + 1,  # startTs is inclusive
        "take": take,
    }
    try:
        result = requests.get(
            f"{url}/records",
            params=params,
            headers={"content-type": "application/json"},
        )
    except KeyboardInterrupt:
        raise
    except Exception as e:
        print(f"Error fetching records: {e}")
        return []
    if result.status_code != 200:
        print(f"Error fetching records: status {result.status_code}")
        return []

    # success!
    records: list[MpuRecord] = result.json()["records"]
    if len(records) == 0:
        return []

    last_mpu_record_ts = records[-1]["ts"]
    return [record["data"] for record in records]


def upload_trajectory_record(data: TrajectoryRecordData) -> bool:
    """Returns True on success and False on error."""
    body = {
        "environmentKey": environment_key,
        "device": trajectory_device,
        "data": data,
    }
    try:
        result = requests.post(
            f"{url}/records",
            json=body,
            headers={"content-type": "application/json"},
        )
    except KeyboardInterrupt:
        raise
    except Exception as e:
        print(f"Error uploading record: {e}")
        return False
    if result.status_code != 200:
        print(f"Error uploading record: status {result.status_code}")
        return False

    # success!
    return True


last_trajectory_message_ts = init_ts


def has_calibration_message() -> bool:
    """Returns false on error."""
    global last_trajectory_message_ts

    params = {
        "environmentKey": environment_key,
        "device": mpu_device,
        "afterTs": last_trajectory_message_ts,
    }
    try:
        result = requests.get(
            f"{url}/messages/next",
            params=params,
            headers={"content-type": "application/json"},
        )
    except KeyboardInterrupt:
        raise
    except Exception as e:
        print(f"Error fetching messages: {e}")
        return False
    if result.status_code != 200:
        print(f"Error fetching messages: status {result.status_code}")
        return False
    if result.text == "NONE":
        return False

    # success!
    message = result.json()
    last_trajectory_message_ts = message["ts"]
    return message["data"] == calibrate_trajectory_message
