#include "interface.h"

#include "hardware.h"
#include "state.h"

namespace interface {

byte getStateByte() {
    using namespace state;

    switch (state::getOpState()) {
        case OpState::fire:
            return 0;
        case OpState::fill:
            return 1;
        case OpState::purge:
            return 2;
        case OpState::abort:
            return 3;
        case OpState::standby:
            return 4;
        case OpState::keep:
            return 5;
        case OpState::pulseFillA:
            return 6;
        case OpState::pulseFillB:
            return 7;
        case OpState::pulseFillC:
            return 8;
        case OpState::pulseVentA:
            return 25;
        case OpState::pulseVentB:
            return 26;
        case OpState::pulseVentC:
            return 27;
        case OpState::pulsePurgeA:
            return 30;
        case OpState::pulsePurgeB:
            return 31;
        case OpState::pulsePurgeC:
            return 32;
        case OpState::fireManualIgniter:
            return 20;
        case OpState::fireManualValve:
            return 21;
        case OpState::custom:
            return 40;
    }
}

byte getRelayStatusByte() {
    using namespace hardware::relay;

    return (getFill() * 1) | (getVent() * 2) | (getPyroCutter() * 4) |
           (getServoValve() * 8) | (getIgniter() * 16);
}

RelayStatus parseRelayStatusByte(byte val) {
    RelayStatus status{
        .fill = val & 1,
        .vent = val & 2,
        .pyroCutter = val & 4,
        .servoValve = val & 8,
        .igniter = val & 16,
    };
    return status;
}

}  // namespace interface
