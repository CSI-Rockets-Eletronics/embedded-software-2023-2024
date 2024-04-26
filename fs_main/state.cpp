#include "state.h"

#include <Arduino.h>

#include "hardware.h"
#include "interface.h"

namespace state {

// in milliseconds
const int PULSE_FILL_A_TIME = 1000;
const int PULSE_FILL_B_TIME = 5000;
const int PULSE_FILL_C_TIME = 10000;

// in milliseconds
const int PULSE_VENT_A_TIME = 1000;
const int PULSE_VENT_B_TIME = 2000;
const int PULSE_VENT_C_TIME = 5000;

// in milliseconds
const int PULSE_PURGE_A_TIME = 1000;
const int PULSE_PURGE_B_TIME = 2000;
const int PULSE_PURGE_C_TIME = 5000;

// in milliseconds
const int FIRE_IGNITER_TIME = 10000;
const int FIRE_IGNITER_VALVE_BUFFER_TIME = 500;
const int FIRE_PYRO_VALVE_TIME = 30000;

// internal state
enum class State {
    // standby
    STANDBY,
    // keep
    KEEP_P_IN_RANGE,
    KEEP_P_ABOVE_MAX,
    KEEP_P_BELOW_MIN,
    // fill
    FILL_P_ABOVE_ABORT,
    FILL_P_BELOW_ABORT,
    // purge
    PURGE_P_ABOVE_ABORT,
    PURGE_P_BELOW_ABORT,
    // pulse-fill-A
    PULSE_FILL_A_P_ABOVE_ABORT,
    PULSE_FILL_A_P_BELOW_ABORT,
    // pulse-fill-B
    PULSE_FILL_B_P_ABOVE_ABORT,
    PULSE_FILL_B_P_BELOW_ABORT,
    // pulse-fill-C
    PULSE_FILL_C_P_ABOVE_ABORT,
    PULSE_FILL_C_P_BELOW_ABORT,
    // pulse-vent-A
    PULSE_VENT_A,
    // pulse-vent-B
    PULSE_VENT_B,
    // pulse-vent-C
    PULSE_VENT_C,
    // pulse-purge-A
    PULSE_PURGE_A_P_ABOVE_ABORT,
    PULSE_PURGE_A_P_BELOW_ABORT,
    // pulse-purge-B
    PULSE_PURGE_B_P_ABOVE_ABORT,
    PULSE_PURGE_B_P_BELOW_ABORT,
    // pulse-purge-C
    PULSE_PURGE_C_P_ABOVE_ABORT,
    PULSE_PURGE_C_P_BELOW_ABORT,
    // fire
    FIRE_IGNITER,
    FIRE_IGNITER_VALVE_BUFFER,
    FIRE_PYRO_VALVE,
    // fire-manual-igniter
    FIRE_MANUAL_IGNITER,
    // fire-manual-valve
    FIRE_MANUAL_VALVE,
    // abort
    ABORT,
    // custom
    CUSTOM,
};

State curState;
interface::RelayStatus customRelayStatus;

// time when current curState was entered
unsigned long enteredStateMillis;

void enterState(State state) {
    curState = state;
    enteredStateMillis = millis();
}

OpState getOpState() {
    switch (curState) {
        case State::STANDBY:
            return OpState::standby;
        case State::KEEP_P_IN_RANGE:
        case State::KEEP_P_ABOVE_MAX:
        case State::KEEP_P_BELOW_MIN:
            return OpState::keep;
        case State::FILL_P_ABOVE_ABORT:
        case State::FILL_P_BELOW_ABORT:
            return OpState::fill;
        case State::PURGE_P_ABOVE_ABORT:
        case State::PURGE_P_BELOW_ABORT:
            return OpState::purge;
        case State::PULSE_FILL_A_P_ABOVE_ABORT:
        case State::PULSE_FILL_A_P_BELOW_ABORT:
            return OpState::pulseFillA;
        case State::PULSE_FILL_B_P_ABOVE_ABORT:
        case State::PULSE_FILL_B_P_BELOW_ABORT:
            return OpState::pulseFillB;
        case State::PULSE_FILL_C_P_ABOVE_ABORT:
        case State::PULSE_FILL_C_P_BELOW_ABORT:
            return OpState::pulseFillC;
        case State::PULSE_VENT_A:
            return OpState::pulseVentA;
        case State::PULSE_VENT_B:
            return OpState::pulseVentB;
        case State::PULSE_VENT_C:
            return OpState::pulseVentC;
        case State::PULSE_PURGE_A_P_ABOVE_ABORT:
        case State::PULSE_PURGE_A_P_BELOW_ABORT:
            return OpState::pulsePurgeA;
        case State::PULSE_PURGE_B_P_ABOVE_ABORT:
        case State::PULSE_PURGE_B_P_BELOW_ABORT:
            return OpState::pulsePurgeB;
        case State::PULSE_PURGE_C_P_ABOVE_ABORT:
        case State::PULSE_PURGE_C_P_BELOW_ABORT:
            return OpState::pulsePurgeC;
        case State::FIRE_IGNITER:
        case State::FIRE_IGNITER_VALVE_BUFFER:
        case State::FIRE_PYRO_VALVE:
            return OpState::fire;
        case State::FIRE_MANUAL_IGNITER:
            return OpState::fireManualIgniter;
        case State::FIRE_MANUAL_VALVE:
            return OpState::fireManualValve;
        case State::ABORT:
            return OpState::abort;
        case State::CUSTOM:
            return OpState::custom;
    }
}

void setOpState(OpState opState) {
    switch (opState) {
        case OpState::standby:
            enterState(State::STANDBY);
            break;
        case OpState::keep:
            enterState(State::KEEP_P_IN_RANGE);
            break;
        case OpState::fill:
            enterState(State::FILL_P_BELOW_ABORT);
            break;
        case OpState::purge:
            enterState(State::PURGE_P_BELOW_ABORT);
            break;
        case OpState::pulseFillA:
            enterState(State::PULSE_FILL_A_P_BELOW_ABORT);
            break;
        case OpState::pulseFillB:
            enterState(State::PULSE_FILL_B_P_BELOW_ABORT);
            break;
        case OpState::pulseFillC:
            enterState(State::PULSE_FILL_C_P_BELOW_ABORT);
            break;
        case OpState::pulseVentA:
            enterState(State::PULSE_VENT_A);
            break;
        case OpState::pulseVentB:
            enterState(State::PULSE_VENT_B);
            break;
        case OpState::pulseVentC:
            enterState(State::PULSE_VENT_C);
            break;
        case OpState::pulsePurgeA:
            enterState(State::PULSE_PURGE_A_P_BELOW_ABORT);
            break;
        case OpState::pulsePurgeB:
            enterState(State::PULSE_PURGE_B_P_BELOW_ABORT);
            break;
        case OpState::pulsePurgeC:
            enterState(State::PULSE_PURGE_C_P_BELOW_ABORT);
            break;
        case OpState::fire:
            enterState(State::FIRE_IGNITER);
            break;
        case OpState::fireManualIgniter:
            enterState(State::FIRE_MANUAL_IGNITER);
            break;
        case OpState::fireManualValve:
            enterState(State::FIRE_MANUAL_VALVE);
            break;
        case OpState::abort:
            enterState(State::ABORT);
            break;
    }

    Serial.print("Set op state: ");
    Serial.println((int)opState);
}

void setOpStateToCustom(byte relayStatusByte) {
    customRelayStatus = interface::parseRelayStatusByte(relayStatusByte);
    enterState(State::CUSTOM);

    Serial.print("Set op state: ");
    Serial.println((int)OpState::custom);
}

void runStateTransition() {
    using namespace hardware;

    unsigned long timeInState = millis() - enteredStateMillis;
    // use top transducer for triggering abort, as bottom transducer sometimes
    // freezes over
    long pressure = hardware::transducer::getSmallTransd2MPSI();

    // conditions for ox tank, including buffer to prevent oscillation

    bool pAboveAbort =
        pressure > transducer::ABORT_MPSI + transducer::BUFFER_MPSI;
    bool pBelowAbort =
        pressure < transducer::ABORT_MPSI - transducer::BUFFER_MPSI;

    bool pAboveMax = pressure > transducer::MAX_MPSI + transducer::BUFFER_MPSI;
    bool pBelowMax = pressure < transducer::MAX_MPSI - transducer::BUFFER_MPSI;

    bool pAboveMin = pressure > transducer::MIN_MPSI + transducer::BUFFER_MPSI;
    bool pBelowMin = pressure < transducer::MIN_MPSI - transducer::BUFFER_MPSI;

    switch (curState) {
        // keep
        case State::KEEP_P_IN_RANGE:
            if (pAboveMax) {
                enterState(State::KEEP_P_ABOVE_MAX);
            } else if (pBelowMin) {
                enterState(State::KEEP_P_BELOW_MIN);
            }
            break;
        case State::KEEP_P_ABOVE_MAX:
            if (pBelowMax) {
                enterState(State::KEEP_P_IN_RANGE);
            }
            break;
        case State::KEEP_P_BELOW_MIN:
            if (pAboveMin) {
                enterState(State::KEEP_P_IN_RANGE);
            }
            break;
        // fill
        case State::FILL_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::FILL_P_BELOW_ABORT);
            }
            break;
        case State::FILL_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::FILL_P_ABOVE_ABORT);
            }
            break;
        // purge
        case State::PURGE_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PURGE_P_BELOW_ABORT);
            }
            break;
        case State::PURGE_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PURGE_P_ABOVE_ABORT);
            }
            break;
        // pulse-fill-A
        case State::PULSE_FILL_A_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PULSE_FILL_A_P_BELOW_ABORT);
            }
            break;
        case State::PULSE_FILL_A_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PULSE_FILL_A_P_ABOVE_ABORT);
            } else if (timeInState > PULSE_FILL_A_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-fill-B
        case State::PULSE_FILL_B_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PULSE_FILL_B_P_BELOW_ABORT);
            }
            break;
        case State::PULSE_FILL_B_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PULSE_FILL_B_P_ABOVE_ABORT);
            } else if (timeInState > PULSE_FILL_B_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-fill-C
        case State::PULSE_FILL_C_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PULSE_FILL_C_P_BELOW_ABORT);
            }
            break;
        case State::PULSE_FILL_C_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PULSE_FILL_C_P_ABOVE_ABORT);
            } else if (timeInState > PULSE_FILL_C_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-vent-A
        case State::PULSE_VENT_A:
            if (timeInState > PULSE_VENT_A_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-vent-B
        case State::PULSE_VENT_B:
            if (timeInState > PULSE_VENT_B_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-vent-C
        case State::PULSE_VENT_C:
            if (timeInState > PULSE_VENT_C_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-purge-A
        case State::PULSE_PURGE_A_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PULSE_PURGE_A_P_BELOW_ABORT);
            }
            break;
        case State::PULSE_PURGE_A_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PULSE_PURGE_A_P_ABOVE_ABORT);
            } else if (timeInState > PULSE_PURGE_A_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-purge-B
        case State::PULSE_PURGE_B_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PULSE_PURGE_B_P_BELOW_ABORT);
            }
            break;
        case State::PULSE_PURGE_B_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PULSE_PURGE_B_P_ABOVE_ABORT);
            } else if (timeInState > PULSE_PURGE_B_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // pulse-purge-C
        case State::PULSE_PURGE_C_P_ABOVE_ABORT:
            if (pBelowAbort) {
                enterState(State::PULSE_PURGE_C_P_BELOW_ABORT);
            }
            break;
        case State::PULSE_PURGE_C_P_BELOW_ABORT:
            if (pAboveAbort) {
                enterState(State::PULSE_PURGE_C_P_ABOVE_ABORT);
            } else if (timeInState > PULSE_PURGE_C_TIME) {
                enterState(State::STANDBY);
            }
            break;
        // fire
        case State::FIRE_IGNITER:
            if (timeInState > FIRE_IGNITER_TIME) {
                enterState(State::FIRE_IGNITER_VALVE_BUFFER);
            }
            break;
        case State::FIRE_IGNITER_VALVE_BUFFER:
            if (timeInState > FIRE_IGNITER_VALVE_BUFFER_TIME) {
                enterState(State::FIRE_PYRO_VALVE);
            }
            break;
        case State::FIRE_PYRO_VALVE:
            if (timeInState > FIRE_PYRO_VALVE_TIME) {
                enterState(State::STANDBY);
            }
            break;
    }
}

void init() {
    // this initializes curState and enteredStateMillis
    enterState(State::STANDBY);
}

void tick() {
    using namespace hardware::relay;

    // run transition first, then update relays
    runStateTransition();

    // first reset relays; note that values aren't flushed to relays until
    // hardware::tick()
    setFill(false);
    setVent(false);
    setAbort(false);
    setPyroCutter(false);
    setServoValve(false);
    setIgniter(false);
    setServoValveAttached(false);

    switch (curState) {
        // standby
        case State::STANDBY:
            break;  // leave all relays off
        // keep
        case State::KEEP_P_IN_RANGE:
            break;  // leave all relays off
        case State::KEEP_P_ABOVE_MAX:
            setVent(true);
            break;
        case State::KEEP_P_BELOW_MIN:
            setFill(true);
            break;
        // fill
        case State::FILL_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::FILL_P_BELOW_ABORT:
            setFill(true);
            break;
        // purge
        case State::PURGE_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PURGE_P_BELOW_ABORT:
            setFill(true);
            setVent(true);
            break;
        // pulse-fill-A
        case State::PULSE_FILL_A_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PULSE_FILL_A_P_BELOW_ABORT:
            setFill(true);
            break;
        // pulse-fill-B
        case State::PULSE_FILL_B_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PULSE_FILL_B_P_BELOW_ABORT:
            setFill(true);
            break;
        // pulse-fill-C
        case State::PULSE_FILL_C_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PULSE_FILL_C_P_BELOW_ABORT:
            setFill(true);
            break;
        // pulse-vent-A
        case State::PULSE_VENT_A:
            setVent(true);
            break;
        // pulse-vent-B
        case State::PULSE_VENT_B:
            setVent(true);
            break;
        // pulse-vent-C
        case State::PULSE_VENT_C:
            setVent(true);
            break;
        // pulse-purge-A
        case State::PULSE_PURGE_A_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PULSE_PURGE_A_P_BELOW_ABORT:
            setFill(true);
            setVent(true);
            break;
        // pulse-purge-B
        case State::PULSE_PURGE_B_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PULSE_PURGE_B_P_BELOW_ABORT:
            setFill(true);
            setVent(true);
            break;
        // pulse-purge-C
        case State::PULSE_PURGE_C_P_ABOVE_ABORT:
            setVent(true);
            break;
        case State::PULSE_PURGE_C_P_BELOW_ABORT:
            setFill(true);
            setVent(true);
            break;
        // fire
        case State::FIRE_IGNITER:
            setIgniter(true);
            setServoValveAttached(true);
            break;
        case State::FIRE_IGNITER_VALVE_BUFFER:
            setServoValveAttached(true);
            break;
        case State::FIRE_PYRO_VALVE:
            setServoValve(true);
            setServoValveAttached(true);
            break;
        // fire-manual-igniter
        case State::FIRE_MANUAL_IGNITER:
            setIgniter(true);
            setServoValveAttached(true);
            break;
        // fire-manual-valve
        case State::FIRE_MANUAL_VALVE:
            setServoValve(true);
            setServoValveAttached(true);
            break;
        // abort
        case State::ABORT:
            setAbort(true);
            break;
        // custom
        case State::CUSTOM:
            setFill(customRelayStatus.fill);
            setVent(customRelayStatus.vent);
            setAbort(customRelayStatus.abort);
            setPyroCutter(customRelayStatus.pyroCutter);
            setIgniter(customRelayStatus.igniter);
            setServoValve(customRelayStatus.servoValve);
            // temporary until we switch away from a servo valve
            setServoValveAttached(customRelayStatus.servoValve);
            break;
    }
}

}  // namespace state