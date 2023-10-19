#ifndef STATE_H_
#define STATE_H_

#include <Arduino.h>

namespace state {

enum class OpState {
    standby,
    keep,
    fill,
    purge,
    pulseFillA,
    pulseFillB,
    pulseFillC,
    pulseVentA,
    pulseVentB,
    pulseVentC,
    pulsePurgeA,
    pulsePurgeB,
    pulsePurgeC,
    fire,
    fireManualIgniter,
    fireManualValve,
    abort,
    custom,
};

OpState getOpState();
void setOpState(OpState newOpState);
void setOpStateToCustom(byte relayStatusByte);
void init();
void tick();

}  // namespace state

#endif  // STATE_H_