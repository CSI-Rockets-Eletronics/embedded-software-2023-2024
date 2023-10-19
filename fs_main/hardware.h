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
bool getPyroValve();
bool getPyroCutter();
bool getIgniter();

void setFill(bool on);
void setVent(bool on);
void setPyroValve(bool on);
void setPyroCutter(bool on);
void setIgniter(bool on);

}  // namespace relay

namespace oxTank {

// milli psi
const long MIN_MPSI = 730000;
const long MAX_MPSI = 770000;
const long ABORT_MPSI = 900000;

// buffer to prevent oscillation
const long BUFFER_MPSI = 2000;

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