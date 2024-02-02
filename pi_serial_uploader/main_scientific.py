import uploader
import struct
import json

delimiter = b"\xAA\x55"  # {0b10101010, 0b01010101}


def parse_packet(packet: bytes) -> str:
    if len(packet) == 8:
        # breakdown of "!ii":
        #   "!": network byte order
        #   "i": int
        bt1, bt2 = struct.unpack("!ii", packet)
        data = {"b1": bt1, "bt2": bt2}
        return json.dumps(data)

    raise ValueError(f"Expected packet length 8, got {len(packet)}")


uploader.run("Scientific", delimiter, parse_packet)
