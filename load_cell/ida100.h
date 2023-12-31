#ifndef IDA100_H
#define IDA100_H

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "ftd2xx.h"

class IDA100 {
   private:
    const int CALIBRATE_SAMPLES = 50;

    const double ticksPerPound;

    FT_HANDLE ftHandle;
    int calibZero;

    void die(std::string msg) {
        std::cout << msg << std::endl;
        exit(1);
    };

    void safe_FT(const char *label, FT_STATUS ftStatus) {
        if (ftStatus != FT_OK) {
            std::string msg = "safe_FT error for " + std::string(label) + ": " +
                              std::to_string(ftStatus);
            die(msg);
        }
    };

   public:
    IDA100(PVOID devIndex, double ticksPerPound)
        : ticksPerPound(ticksPerPound) {
        // read serial number of device at devIndex
        char serialNumber[64];
        safe_FT("FT_ListDevices",
                FT_ListDevices(devIndex, serialNumber,
                               FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER));

        // open device by serial number and store into this.ftHandle
        std::cout << "Opening device with serial number: " << serialNumber
                  << std::endl;
        safe_FT("FT_OpenEx", FT_OpenEx((PVOID)serialNumber,
                                       FT_OPEN_BY_SERIAL_NUMBER, &ftHandle));

        // set device parameters (snooped from FUTEK_USB_DLL)
        safe_FT("FT_SetTimeouts", FT_SetTimeouts(ftHandle, 500, 500));
        safe_FT("FT_SetUSBParameters", FT_SetUSBParameters(ftHandle, 8, 8));
        safe_FT("FT_SetLatencyTimer", FT_SetLatencyTimer(ftHandle, 2));
    }

    void close() { safe_FT("FT_Close", FT_Close(ftHandle)); }

    ~IDA100() { close(); }

    int readRawData() {
        DWORD bytesWritten;
        DWORD bytesRead;
        unsigned char readBuffer[8];

        // phase 1: no meaningful data is retrieved, but FUTEK_USB_DLL does
        // it, and things don't work without it

        // write these particular 4 bytes
        unsigned char p1BytesToWrite[] = {0b00000001, 0b00000100, 0b01000100,
                                          0b10101010};
        safe_FT("FT_Write",
                FT_Write(ftHandle, p1BytesToWrite, 4, &bytesWritten));
        if (bytesWritten != 4) {
            die("phase 1 FT_Write: bytesWritten != 4");
        }

        // read and ignore the next 8 bytes (they seem to be the same each time)
        safe_FT("FT_Read", FT_Read(ftHandle, readBuffer, 8, &bytesRead));
        if (bytesRead != 8) {
            die("phase 1 FT_Read: bytesRead != 8");
        }

        // phase 2: do another unknown write + read, but this time, the data is
        // found in the read

        // write these particular 4 bytes
        unsigned char p2BytesToWrite[] = {0b00000010, 0b00000100, 0b01000010,
                                          0b10101010};
        safe_FT("FT_Write",
                FT_Write(ftHandle, p2BytesToWrite, 4, &bytesWritten));
        if (bytesWritten != 4) {
            die("phase 1 FT_Write: bytesWritten != 4");
        }

        // read the next 8 bytes
        safe_FT("FT_Read", FT_Read(ftHandle, readBuffer, 8, &bytesRead));
        if (bytesRead != 8) {
            die("phase 1 FT_Read: bytesRead != 8");
        }

        // the data we want is in bytes 4 to 6
        // (the other bytes seem to never change)
        int data = readBuffer[4] << 16 | readBuffer[5] << 8 | readBuffer[6];
        return data;
    }

    int readCalibratedData() { return readRawData() - calibZero; }

    void calibrate() {
        std::vector<int> samples;
        for (int i = 0; i < CALIBRATE_SAMPLES; i++) {
            samples.push_back(readRawData());
        }

        // take median
        std::sort(samples.begin(), samples.end());
        calibZero = samples[CALIBRATE_SAMPLES / 2];
    }

    double readLbs() { return readCalibratedData() / ticksPerPound; }
};

#endif  // IDA100_H