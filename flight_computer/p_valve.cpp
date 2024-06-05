#include "p_valve.h"

#include <Arduino.h>

namespace pValve {

/**
 * Input is pulled HIGH by default. Output relay starts off. When input is read
 * as LOW, the relay is turned on. when the input is read as HIGH and the relay
 * was previously on, the relay is turned off after a lag time.
 */

const int INPUT_PIN = 14;
const int RELAY_PIN = 15;

const long LAG_TIME_MS = 30 * 1000;  // 30 seconds

long lastRelayOnTimeMs = -1;  // negative means the relay has never been on

void setup() {
    pinMode(INPUT_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
    long nowMs = millis();
    int input = digitalRead(INPUT_PIN);

    if (input == LOW) {
        // input is LOW, so turn on the relay
        digitalWrite(RELAY_PIN, HIGH);
        lastRelayOnTimeMs = nowMs;
        return;
    }

    // else: input is HIGH

    if (lastRelayOnTimeMs < 0) {
        // relay has never been on
        digitalWrite(RELAY_PIN, LOW);
        return;
    }

    // else: relay was previously on

    if (nowMs - lastRelayOnTimeMs < LAG_TIME_MS) {
        // relay was recently turned on, so keep it on
        digitalWrite(RELAY_PIN, HIGH);
    } else {
        // relay was turned on a long time ago, so turn it off
        digitalWrite(RELAY_PIN, LOW);
    }
}

}  // namespace pValve