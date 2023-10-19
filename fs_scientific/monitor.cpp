#include "monitor.h"

#include <TickTwo.h>

#include "hardware.h"

namespace monitor {

int updateCount = 0;

void incrementUpdateCount() { updateCount++; }

void logUpdateFrequency() {
    hardware::usbSerial::debugPrint("Update frequency: ");
    hardware::usbSerial::debugPrint(updateCount);
    hardware::usbSerial::debugPrintln(" Hz");
    updateCount = 0;
}

TickTwo logUpdateFrequencyTicker(logUpdateFrequency, 1000);

void init() { logUpdateFrequencyTicker.start(); }

void tick() { logUpdateFrequencyTicker.update(); }

}  // namespace monitor