import serial
import numpy as np
from scipy.fft import fft,fftfreq
import matplotlib.pyplot as plt

AX=[]
AY=[]
AZ=[]
GX=[]
GY=[]
GZ=[]
MX=[]
MY=[]
MZ=[]

t = [0]
ax = [0]
ay = [0]
az = [0]
gx = [0]
gy = [0]
gz = [0]
mx = [0]
my = [0]
mz = [0]

plt.ion()

def parse_data(data):
    result = {}
    for s in data.split(","):
        if not s:
            continue
        key, value = s.split(":")
        result[key] = int(value)
    return result

def tolists(data):
    AX.append(parse_data(data)["AX"])
    AY.append(parse_data(data)["AY"])
    AZ.append(parse_data(data)["AZ"])
    GX.append(parse_data(data)["GX"])
    GY.append(parse_data(data)["GY"])
    GZ.append(parse_data(data)["GZ"])
    MX.append(parse_data(data)["MX"])
    MY.append(parse_data(data)["MY"])
    MZ.append(parse_data(data)["MZ"])

def appendlists(t, ax, ay, az, gx, gy, gz, mx, my, mz):
    t.append(t[-1] + 1)
    ax.append(AX[-1])
    ay.append(AY[-1])
    az.append(AZ[-1])
    gx.append(GX[-1])
    gy.append(GY[-1])
    gz.append(GZ[-1])
    mx.append(MX[-1])
    my.append(MY[-1])
    mz.append(MZ[-1])

def plot(t, ax, ay, az, gx, gy, gz, mx, my, mz):
    plt.cla()
    plt.plot(t, ax, label="ax")
    plt.plot(t, ay, label="ay")
    plt.plot(t, az, label="az")
    plt.plot(t, gx, label="gx")
    plt.plot(t, gy, label="gy")
    plt.plot(t, gz, label="gz")
    plt.plot(t, mx, label="mx")
    plt.plot(t, my, label="my")
    plt.plot(t, mz, label="mz")
    plt.pause(0.1)

def readserial(comport, baudrate):
    ser = serial.Serial(
        comport, baudrate, timeout=0.1
    )  # 1/timeout is the frequency at which the port is read

    while True:
        data = ser.readline().decode()
        tolists(data)
        appendlists(t, ax, ay, az, gx, gy, gz, mx, my, mz)
        #plot(t, ax, ay, az, gx, gy, gz, mx, my, mz)
        N=len(t)
        yf = fft(gx)
        xf = fftfreq(N, 0.1)[:N//2]
        plt.cla()
        plt.plot(xf, 2.0/N * np.abs(yf[0:N//2]))
        plt.show()
        plt.pause(1)

readserial("COM6", 115200)

