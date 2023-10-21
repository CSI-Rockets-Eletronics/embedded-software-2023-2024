import serial

class SensorData:
    acc = list()
    gyro = list()
    mag = list()

def parseLine(line: str) -> SensorData:
    cleaned = line.strip().split(",")
    print(cleaned)
    parsed = SensorData()
    parsed.acc = []
    parsed.acc[0] = int(cleaned[0].replace("AX: ", ""))
    parsed.acc[1] = int(cleaned[0].replace("AY: ", ""))
    parsed.acc[2] = int(cleaned[0].replace("AZ: ", ""))
    parsed.gyro[0] = int(cleaned[0].replace("GX: ", ""))
    parsed.gyro[1] = int(cleaned[0].replace("GY: ", ""))
    parsed.gyro[2] = int(cleaned[0].replace("GZ: ", ""))
    parsed.mag[0] = int(cleaned[0].replace("MX: ", ""))
    parsed.mag[1] = int(cleaned[0].replace("MY: ", ""))
    parsed.mag[2] = int(cleaned[0].replace("MZ: ", ""))
    return parsed

def readserial(comport, baudrate):
    ser = serial.Serial(
        comport, baudrate, timeout=0.1
    )  # 1/timeout is the frequency at which the port is read

    while True:
        data = ser.readline().decode()
        data = parseLine(data)        
        

readserial("COM6", 115200)
