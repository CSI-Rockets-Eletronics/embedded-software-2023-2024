import uploader
import struct
import json

delimiter = b"\xAA\x55"  # {0b10101010, 0b01010101}


def parse_packet(packet: bytes) -> str:
    if len(packet) == 23:
        # breakdown of "<QBBBiif":
        #   "<": little-endian
        #   "Q": uint64_t (8 bytes)
        #   "B": uint8_t (1 byte)
        #   "B": uint8_t (1 byte)
        #   "B": uint8_t (1 byte)
        #   "i": int32_t (4 bytes)
        #   "i": int32_t (4 bytes)
        #   "f": float (4 bytes)
        (
            ts,
            gps_fix,
            gps_fixquality,
            gps_satellites,
            gps_latitude_fixed,
            gps_longitude_fixed,
            gps_altitude,
        ) = struct.unpack("<QBBBiif", packet)
        data = {
            "ts": ts,
            "gps_fix": gps_fix,
            "gps_fixquality": gps_fixquality,
            "gps_satellites": gps_satellites,
            "gps_latitude_fixed": gps_latitude_fixed,
            "gps_longitude_fixed": gps_longitude_fixed,
            "gps_altitude": gps_altitude,
        }
        return json.dumps(data)

    raise ValueError(f"Expected packet length 23, got {len(packet)}")


uploader.run("Rocket", delimiter, parse_packet)
