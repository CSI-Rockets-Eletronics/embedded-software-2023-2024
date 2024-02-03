#ifndef HARDWARE_H_
#define HARDWARE_H_

#include <Arduino.h>

namespace hardware {

namespace sciSerial {

void sendRecalibrateCommand();
void sendClearCalibrationCommand();

}  // namespace sciSerial

namespace relay {

bool getFill();
bool getVent();
bool getPyroCutter();
bool getIgniter();

bool getServoValve();

void setFill(bool on);
void setVent(bool on);
void setPyroCutter(bool on);
void setIgniter(bool on);

void setServoValve(bool on);
void setServoValveAttached(bool attached);

}  // namespace relay

namespace oxTank {

// milli psi
const long MIN_MPSI = 730000;
const long MAX_MPSI = 770000;
const long ABORT_MPSI = 900000;

// buffer to prevent oscillation
const long BUFFER_MPSI = 5000;

long getMPSI();

}  // namespace oxTank

namespace combustionChamber {

long getMPSI();

}  // namespace combustionChamber

int64_t getCalibrationTime();

void init();
void tick();

}  // namespace hardware

#endif  // HARDWARE_H_