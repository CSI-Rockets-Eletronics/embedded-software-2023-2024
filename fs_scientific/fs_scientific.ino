#include "hardware.h"
#include "monitor.h"

void setup() {
    hardware::init();
    monitor::init();
}

void loop() {
    hardware::tick();
    monitor::tick();
}
