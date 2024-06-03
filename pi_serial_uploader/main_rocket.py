import uploader
import struct
import json

delimiter = b"\xAA\x55"  # {0b10101010, 0b01010101}


def parse_device(packet: bytes) -> str:
    if len(packet) == 21:
        return "RocketScientific"
    if len(packet) == 20:
        return "MPU"
    if len(packet) == 16:
        return "DHT"
    raise ValueError(f"Expected packet length 16 or 20, got {len(packet)}")


def parse_packet(packet: bytes) -> str:
    if len(packet) == 21:
        # breakdown of "!qiiic":
        #   "!": network byte order
        #   "q": long long
        #   "i": int
        #   "c": char
        # the last byte is just to make the packet 21 bytes long
        ts, t1, t2, t3, _c = struct.unpack("!qiii", packet)
        data = {"ts": ts, "t1": t1, "t2": t2, "t3": t3}
        return json.dumps(data)
    if len(packet) == 20:
        # breakdown of "!qhhhhhh":
        #   "!": network byte order
        #   "q": long long
        #   "h": short
        ts, ax, ay, az, gx, gy, gz = struct.unpack("!qhhhhhh", packet)
        data = {"ts": ts, "ax": ax, "ay": ay, "az": az, "gx": gx, "gy": gy, "gz": gz}
        return json.dumps(data)

    if len(packet) == 16:
        # breakdown of "!qff":
        #   "!": network byte order
        #   "q": long long
        #   "f": float
        ts, temp, hum = struct.unpack("!qff", packet)
        data = {"ts": ts, "temp": temp, "hum": hum}
        return json.dumps(data)

    raise ValueError(f"Expected packet length 16, 20, or 21, got {len(packet)}")


uploader.run(parse_device, delimiter, parse_packet)
