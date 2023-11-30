#ifndef FREQUENCY_LOGGER_H_
#define FREQUENCY_LOGGER_H_

#include <Arduino.h>

class FrequencyLogger {
   public:
    FrequencyLogger(String label, long printFrequencyMs)
        : label(label),
          printFrequencyMs(printFrequencyMs),
          lastPrintTime(millis()),
          tickCount(0) {}

    void tick() {
        tickCount++;

        if (millis() - lastPrintTime > printFrequencyMs) {
            Serial.print("[");
            Serial.print(label);
            Serial.print("] Frequency: ");
            Serial.print(tickCount);
            Serial.println(" Hz");
            lastPrintTime = millis();
            tickCount = 0;
        }
    }

   private:
    String label;
    unsigned long printFrequencyMs;
    unsigned long lastPrintTime;
    unsigned long tickCount;
};

#endif  // FREQUENCY_LOGGER_H_
