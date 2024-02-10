#ifndef IDA100_H
#define IDA100_H

#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "ftd2xx.h"

static const int CALIBRATE_SAMPLES = 50;

class IDA100 {
   private:
    FT_HANDLE ftHandle;
    int calibZero;

    void die(std::string msg) {
        std::cerr << msg << std::endl;
        exit(1);
    };

    void safe_FT(const char* label, FT_STATUS ftStatus) {
        if (!FT_SUCCESS(ftStatus)) {
            std::string msg = "safe_FT error for " + std::string(label) + ": " +
                              std::to_string(ftStatus);
            die(msg);
        }
    };

    void read(PVOID buf, DWORD count) {
        while (count > 0) {
            DWORD bytesRead;
            safe_FT("FT_Read", FT_Read(ftHandle, buf, count, &bytesRead));
            count -= bytesRead;
        }
    }

    void write(LPVOID buf, DWORD count) {
        DWORD bytesWritten;
        safe_FT("FT_Write", FT_Write(ftHandle, buf, count, &bytesWritten));

        if (bytesWritten != count) {
            std::string msg =
                "FT_Write: bytesWritten != " + std::to_string(count);
            die(msg);
        }
    }

    int readRawData() {
        unsigned char readBuffer[8];

        // phase 1: no meaningful data is retrieved, but FUTEK_USB_DLL does
        // it, and things don't work without it

        // write these particular 4 bytes
        unsigned char p1BytesToWrite[] = {0b00000001, 0b00000100, 0b01000100,
                                          0b10101010};
        write(p1BytesToWrite, 4);

        // read and ignore the next 8 bytes (they seem to be the same each time)
        read(readBuffer, 8);

        // phase 2: do another unknown write + read, but this time, the data is
        // found in the read

        // write these particular 4 bytes
        unsigned char p2BytesToWrite[] = {0b00000010, 0b00000100, 0b01000010,
                                          0b10101010};
        write(p2BytesToWrite, 4);

        // read the next 8 bytes
        read(readBuffer, 8);

        // the data we want is in bytes 4 to 6
        // (the other bytes seem to never change)
        int data = readBuffer[4] << 16 | readBuffer[5] << 8 | readBuffer[6];
        return data;
    }

   public:
    void open(const char* serialNumber) {
        // open device by serial number and store into this.ftHandle
        std::cerr << "Opening device with serial number: " << serialNumber
                  << std::endl;
        safe_FT("FT_OpenEx", FT_OpenEx((PVOID)serialNumber,
                                       FT_OPEN_BY_SERIAL_NUMBER, &ftHandle));

        safe_FT("FT_ResetDevice", FT_ResetDevice(ftHandle));

        // set device parameters (snooped from FUTEK_USB_DLL)
        safe_FT("FT_SetTimeouts", FT_SetTimeouts(ftHandle, 500, 500));
        safe_FT("FT_SetUSBParameters", FT_SetUSBParameters(ftHandle, 8, 8));
        safe_FT("FT_SetLatencyTimer", FT_SetLatencyTimer(ftHandle, 2));

        usleep(1000);
    }

    void close() { safe_FT("FT_Close", FT_Close(ftHandle)); }

    int read() { return readRawData() - calibZero; }

    void calibrate() {
        std::vector<int> samples;
        for (int i = 0; i < CALIBRATE_SAMPLES; i++) {
            samples.push_back(readRawData());
        }

        // take median
        std::sort(samples.begin(), samples.end());
        calibZero = samples[CALIBRATE_SAMPLES / 2];
    }
};

#endif  // IDA100_H