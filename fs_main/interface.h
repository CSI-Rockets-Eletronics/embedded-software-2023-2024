#ifndef STATION_INTERFACE_H_
#define STATION_INTERFACE_H_

#include <Arduino.h>

namespace interface {

struct RelayStatus {
    bool fill;
    bool vent;
    bool pyroCutter;
    bool servoValve;
    bool igniter;
};

#define STANDBY_COMMAND "standby"
#define KEEP_COMMAND "keep"
#define FILL_COMMAND "fill"
#define PURGE_COMMAND "purge"
#define PULSE_FILL_A_COMMAND "pulse-fill-A"
#define PULSE_FILL_B_COMMAND "pulse-fill-B"
#define PULSE_FILL_C_COMMAND "pulse-fill-C"
#define PULSE_VENT_A_COMMAND "pulse-vent-A"
#define PULSE_VENT_B_COMMAND "pulse-vent-B"
#define PULSE_VENT_C_COMMAND "pulse-vent-C"
#define PULSE_PURGE_A_COMMAND "pulse-purge-A"
#define PULSE_PURGE_B_COMMAND "pulse-purge-B"
#define PULSE_PURGE_C_COMMAND "pulse-purge-C"
#define FIRE_COMMAND "fire"
#define FIRE_MANUAL_IGNITER_COMMAND "fire-manual-igniter"
#define FIRE_MANUAL_VALVE_COMMAND "fire-manual-valve"
#define ABORT_COMMAND "abort"
#define CUSTOM_COMMAND "custom"

#define RECALIBRATE_COMMAND "recalibrate"
#define CLEAR_CALIBRATION_COMMAND "clear-calibration"

byte getStateByte();
byte getRelayStatusByte();
RelayStatus parseRelayStatusByte(byte val);

}  // namespace interface

#endif  // STATION_INTERFACE_H_