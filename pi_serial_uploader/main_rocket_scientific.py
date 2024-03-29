import uploader
import struct
import json
from typing import Any

delimiter = b"\xAA\x55"  # {0b10101010, 0b01010101}


def parse_packet(packet: bytes) -> str:
    if len(packet) == 8:
        # breakdown of "!ii":
        #   "!": network byte order
        #   "i": int
        bt1, bt2 = struct.unpack("!ii", packet)
        data = {"bt1": bt1, "bt2": bt2}
        return json.dumps(data)

    raise ValueError(f"Expected packet length 8, got {len(packet)}")


def format_message(message: Any) -> bytes:
    sentence = f"<{message}>"
    return sentence.encode("utf-8")


uploader.run("RocketScientific", delimiter, parse_packet, format_message)
