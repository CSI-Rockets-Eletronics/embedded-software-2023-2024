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
bool getAbort();
bool getPyroCutter();
bool getIgniter();
bool getPValve();

void setFill(bool on);
void setVent(bool on);
void setAbort(bool on);
void setPyroCutter(bool on);
void setIgniter(bool on);
void setPValve(bool on);

}  // namespace relay

namespace thermocouple {

double getThermo1Celcius();
double getThermo2Celcius();

}  // namespace thermocouple

namespace transducer {

// for filling sequence; milli psi
const long MIN_MPSI = 730000;
const long MAX_MPSI = 770000;
const long ABORT_MPSI = 900000;

// buffer to prevent oscillation
const long BUFFER_MPSI = 5000;

long getTransd1MPSI();
long getTransd2MPSI();

}  // namespace transducer

int64_t getCalibrationTime();

void init();
void tick();

}  // namespace hardware

#endif  // HARDWARE_H_