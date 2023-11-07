#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <Arduino.h>

namespace hardware {

// to raspberry pi or computer (for debugging)
namespace usbSerial {

const bool PRINT_DEBUG_TO_SERIAL = true;

template <typename T>
void debugPrint(T value) {
    if (PRINT_DEBUG_TO_SERIAL) {
        Serial.print(value);
    }
}

template <typename T>
void debugPrintln(T value) {
    if (PRINT_DEBUG_TO_SERIAL) {
        Serial.println(value);
    }
}

}  // namespace usbSerial

void recalibrate();
void clearCalibration();
void init();
void tick();

}  // namespace hardware

#endif  // HARDWARE_H_