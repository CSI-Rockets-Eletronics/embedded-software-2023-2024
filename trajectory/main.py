from typing import Any, Optional, cast
import math
import numpy as np
import numpy.typing as npt
from server_utils import (
    fetch_new_mpu_records,
    MpuRecordData,
    TrajectoryRecordData,
    upload_trajectory_record,
    has_calibration_message,
)
from dataclasses import dataclass
import quaternion  # type: ignore


@dataclass
class MpuReading:
    """
    In SI units:
        ts: seconds
        ax,ay,az: m/s^2
        gx,gy,gz: rad/s

    Axes are relative to the rocket's frame:
        x: screw switch access side ("front")
        y: z cross x (observer's right, when facing the screw switches)
        z: tip of the rocket ("top")
    """

    ts: float
    ax: float
    ay: float
    az: float
    gx: float
    gy: float
    gz: float


def to_mpu_reading(data: MpuRecordData) -> MpuReading:
    G = 9.81
    ACC_TICKS_PER_G = 2048
    GYRO_TICKS_PER_DEG = 16
    RAD_PER_DEG = math.pi / 180

    ax, ay, az = data["ax"], data["ay"], data["az"]
    gx, gy, gz = data["gx"], data["gy"], data["gz"]

    # convert into world coordinates
    ax, ay, az = ax, az, -ay
    gx, gy, gz = gx, gz, -gy

    return MpuReading(
        ts=data["ts"] / 1e6,
        ax=ax * G / ACC_TICKS_PER_G,
        ay=ay * G / ACC_TICKS_PER_G,
        az=az * G / ACC_TICKS_PER_G,
        gx=gx * RAD_PER_DEG / GYRO_TICKS_PER_DEG,
        gy=gy * RAD_PER_DEG / GYRO_TICKS_PER_DEG,
        gz=gz * RAD_PER_DEG / GYRO_TICKS_PER_DEG,
    )


@dataclass
class Acceleration:
    ax: float
    ay: float
    az: float


def calibrate_stationary_acc() -> Acceleration:
    STATIONARY_CALIB_SAMPLES = 2000

    calib_samples: list[MpuReading] = []

    while len(calib_samples) < STATIONARY_CALIB_SAMPLES:
        take = STATIONARY_CALIB_SAMPLES - len(calib_samples)
        data = fetch_new_mpu_records(take)
        calib_samples.extend(map(to_mpu_reading, data))

    avg_ax = np.mean([r.ax for r in calib_samples])
    avg_ay = np.mean([r.ay for r in calib_samples])
    avg_az = np.mean([r.az for r in calib_samples])

    return Acceleration(
        ax=float(avg_ax),
        ay=float(avg_ay),
        az=float(avg_az),
    )


Quaternion = Any  # quaternion has no type hints :(


def rotation_from_stationary_acc(acc: Acceleration) -> Quaternion:
    """
    Compute the starting rotation of the rocket. Returns a quaternion
    representing the rotation from the rocket's internal frame to the world
    frame.

    We assume the rocket starts with its internal frame aligned with the world
    frame, e.g. pointing straight upward and rotated such that the internal x
    axis is aligned with the a well-known direction designated as the world's x
    axis.

    From this position, we assume the rocket is "tilted" in some direction away
    from the world's z ("up") axis, but is not rotated around it's internal z
    axis. The returned quaternion is the unique rotation adhering to the above
    constraints that explains the measured gravity acceleration vector.
    """

    # Recall that acc, the gravity acceleration vector, points in the direction
    # of true "up" in the rocket frame. This is because acc=0 represents free
    # fall, and being stationary is equivalent to accelerating upward relative
    # to free fall.

    # Let us represent the rocket's internal "up" vector in the world frame
    # using spherical coordinates, with theta as the azimuthal angle and phi
    # as the polar angle (e.g. angle from the world's z axis).
    anorm = math.sqrt(acc.ax**2 + acc.ay**2 + acc.az**2)
    assert anorm > 0, "Got zero acceleration vector!"
    phi = math.acos(acc.az / anorm)
    # the rocket tilting in one direction results in the gravity vector
    # tilting in the opposite direction (+180 degrees)
    theta = math.atan2(acc.ay, acc.ax) + math.pi

    # Get the rotation vector representation. We could use Euler angles, but
    # they're a world of pain (and confusing conventions)...

    # The "tilt" is equivalent to picking the vector pointing along theta + 90
    # degrees in the xy plane, and rotating about it by phi (per right-hand
    # rule).
    rot_vector_theta = theta + math.pi / 2
    # Recall that a rotation vector encodes the rotation amount in radians
    # as the vector's magnitude.
    rot_vector = np.array([math.cos(rot_vector_theta), math.sin(rot_vector_theta), 0])
    rot_vector *= phi

    # Convert the rotation vector to a quaternion
    return quaternion.from_rotation_vector(rot_vector)  # type: ignore


@dataclass
class State:
    """
    pos: x,y,z position in meters, in the world frame.
    vel: vx,vy,vz velocity in m/s, in the world frame.
    acc: ax,ay,az acceleration in m/s^2, in the world frame.
    rot: rotation quaternion from the rocket frame to the world frame.
    ts: timestamp in seconds of the last update. None on initialization.
    """

    pos: npt.NDArray[np.float64]
    vel: npt.NDArray[np.float64]
    acc: npt.NDArray[np.float64]
    rot: Quaternion
    ts: Optional[float] = None


def to_trajectory_record_data(state: State) -> TrajectoryRecordData | None:
    if state.ts is None:
        return None

    return TrajectoryRecordData(
        ts=int(state.ts * 1e6),
        x=float(state.pos[0]),
        y=float(state.pos[1]),
        z=float(state.pos[2]),
        vx=float(state.vel[0]),
        vy=float(state.vel[1]),
        vz=float(state.vel[2]),
        ax=float(state.acc[0]),
        ay=float(state.acc[1]),
        az=float(state.acc[2]),
    )


"""
Main loop below:
"""

# assume rocket is pointing straight up at the start (until calibrated)
initial_3vec = np.array([0.0, 0.0, 0.0])
initial_rot = quaternion.one  # type: ignore

state = State(
    pos=initial_3vec,
    vel=initial_3vec,
    acc=initial_3vec,
    rot=initial_rot,
)


# assume g=9.81 (until calibrated)
g_magnitude = 9.81  # m/s^2


def step(reading: MpuReading) -> None:
    """Modifies state in-place."""

    # first reading?
    if state.ts is None:
        state.ts = reading.ts
        return

    # compute time step
    dt = reading.ts - state.ts
    state.ts = reading.ts

    # compute acceleration in world frame
    rot_matrix = cast(npt.NDArray[np.float64], quaternion.as_rotation_matrix(state.rot))  # type: ignore
    acc = rot_matrix @ np.array([reading.ax, reading.ay, reading.az])
    acc[2] -= g_magnitude  # subtract gravity

    # update position and velocity
    dvel = acc * dt
    dpos = state.vel * dt + 0.5 * acc * dt**2
    state.vel += dvel
    state.pos += dpos

    # update rotation
    gvec = np.array([reading.gx, reading.gy, reading.gz])
    gvec_norm = np.linalg.norm(gvec)
    if gvec_norm > 0:  # avoid division by zero
        theta = gvec_norm * dt
        rot_axis = gvec / gvec_norm
        quat = quaternion.from_float_array(  # type: ignore
            [math.cos(theta / 2), *(math.sin(theta / 2) * rot_axis)]
        )
        state.rot = (state.rot * quat).normalized()  # type: ignore


while True:
    if has_calibration_message():
        print("Calibrating...")
        stat_acc = calibrate_stationary_acc()
        state = State(
            pos=initial_3vec,
            vel=initial_3vec,
            acc=initial_3vec,
            rot=rotation_from_stationary_acc(stat_acc),
        )
        g_magnitude = math.sqrt(stat_acc.ax**2 + stat_acc.ay**2 + stat_acc.az**2)
        print("Calibrated!")

    records = fetch_new_mpu_records()

    if len(records) == 0:
        continue  # to skip uploading

    for entry in records:
        reading = to_mpu_reading(entry)
        step(reading)

    # upload just the last state
    trajectory_data = to_trajectory_record_data(state)
    if trajectory_data is not None:
        upload_trajectory_record(trajectory_data)
        print("Uploaded record:", trajectory_data)
