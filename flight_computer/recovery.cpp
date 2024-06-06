
#include "recovery.h"

#include <Arduino.h>

namespace recovery {

int MAIN_CHUTE_PIN = 15;

int DEPLOY_MAIN_DELAY_MS = 30 * 1000;  // 40 seconds

void setup() { pinMode(MAIN_CHUTE_PIN, OUTPUT); }

void loop(float acc_total) {
    // TODO implement actual logic

    // for now, dummy logic

    long nowMs = millis();

    if (nowMs > DEPLOY_MAIN_DELAY_MS) {
        digitalWrite(MAIN_CHUTE_PIN, HIGH);
    } else {
        digitalWrite(MAIN_CHUTE_PIN, LOW);
    }
}

}  // namespace recovery