import uploader
import struct
import json

delimiter = b"\xAA\x55"  # {0b10101010, 0b01010101}


def parse(packet: bytes) -> str:
    if len(packet) != 20:
        raise ValueError(f"Expected packet length 20, got {len(packet)}")

    # breakdown of "!qhhhhhh":
    #   "!": network byte order
    #   "q": long long
    #   "h": short
    ts, ax, ay, az, gx, gy, gz = struct.unpack("!qhhhhhh", packet)
    data = {"ts": ts, "ax": ax, "ay": ay, "az": az, "gx": gx, "gy": gy, "gz": gz}
    return json.dumps(data)


uploader.run("FlightComputer", delimiter, parse)
