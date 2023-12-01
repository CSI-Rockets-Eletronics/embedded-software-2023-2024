#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <Arduino.h>

namespace hardware {

void recalibrate();
void clearCalibration();
void init();
void tick();

}  // namespace hardware

#endif  // HARDWARE_H_