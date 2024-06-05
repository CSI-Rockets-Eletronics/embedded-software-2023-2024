import uploader
import struct
import json

delimiter = b"\xAA\x55"  # {0b10101010, 0b01010101}


def parse_packet(packet: bytes) -> str:
    if len(packet) == 16:
        # breakdown of "!qii":
        #   "!": network byte order
        #   "q": long long
        #   "i": int
        ts, t1, t2 = struct.unpack("!qii", packet)
        data = {"ts": ts, "t1": t1, "t2": t2}
        return json.dumps(data)

    raise ValueError(f"Expected packet length 16, got {len(packet)}")


uploader.run("Scientific", delimiter, parse_packet)
