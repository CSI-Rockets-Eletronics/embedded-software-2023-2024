import json
import serial

ser = serial.Serial("/dev/ttyS0", 115200)

while True:
    line = ser.readline()
    # record_json = json.loads(line)
    print(line)
