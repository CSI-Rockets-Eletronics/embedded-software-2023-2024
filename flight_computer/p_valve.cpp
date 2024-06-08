#include "p_valve.h"

#include <Arduino.h>

namespace pValve {

const int INPUT_PIN = 6;
const int RELAY_PIN = 14;

// keep relay on for 30 seconds after the last command to turn it off
const long LAG_TIME_ON_MS = 30 * 1000;  // 30 seconds
// keep relay off for 0.5 seconds after the last command to turn it on
const long LAG_TIME_OFF_MS = 500;  // 0.5 seconds

// pretend the relay was last on a very long time ago
long lastRelayOnCommandTime = -1000 * 1000;
// relay starts off
long lastRelayOffCommandTime = 0;

void setup() {
    pinMode(INPUT_PIN, INPUT_PULLDOWN);
    pinMode(RELAY_PIN, OUTPUT);
}

void loop() {
    long nowMs = millis();
    int input = digitalRead(INPUT_PIN);

    // HIGH -> command to turn relay on
    if (input == HIGH) {
        lastRelayOnCommandTime = nowMs;
        if (nowMs - lastRelayOffCommandTime > LAG_TIME_OFF_MS) {
            digitalWrite(RELAY_PIN, HIGH);
        } else {
            digitalWrite(RELAY_PIN, LOW);
        }
    } else {
        lastRelayOffCommandTime = nowMs;
        if (nowMs - lastRelayOnCommandTime > LAG_TIME_ON_MS) {
            digitalWrite(RELAY_PIN, LOW);
        } else {
            digitalWrite(RELAY_PIN, HIGH);
        }
    }
}

}  // namespace pValve