
#include "recovery.h"

#include <Arduino.h>

namespace recovery {

int DROGUE_CHUTE_PIN = 6;
int MAIN_CHUTE_PIN = 7;

int DEPLOY_DROGUE_DELAY_MS = 30 * 1000;  // 30 seconds
int DEPLOY_MAIN_DELAY_MS = 40 * 1000;    // 40 seconds

void setup() {
    pinMode(DROGUE_CHUTE_PIN, OUTPUT);
    pinMode(MAIN_CHUTE_PIN, OUTPUT);
}

void loop(float acc_total) {
    // TODO implement actual logic

    // for now, dummy logic

    long nowMs = millis();

    if (nowMs > DEPLOY_DROGUE_DELAY_MS) {
        digitalWrite(DROGUE_CHUTE_PIN, HIGH);
    } else {
        digitalWrite(DROGUE_CHUTE_PIN, LOW);
    }

    if (nowMs > DEPLOY_MAIN_DELAY_MS) {
        digitalWrite(MAIN_CHUTE_PIN, HIGH);
    } else {
        digitalWrite(MAIN_CHUTE_PIN, LOW);
    }
}

}  // namespace recovery