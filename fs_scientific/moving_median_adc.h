#ifndef MOVING_MEDIAN_ADC_H_
#define MOVING_MEDIAN_ADC_H_

#include <Adafruit_ADS1X15.h>

#include "utils.h"

class MovingMedianADC {
   public:
    MovingMedianADC(const char *debugName, int windowSize, float defaultZero,
                    std::function<long(float)> convertVoltsToMPSI,
                    bool singleEnded, bool flipVolts = false)
        : debugName(debugName),
          singleEnded(singleEnded),
          flipVolts(flipVolts),
          defaultZero(defaultZero),
          zero(defaultZero),
          medianVolts(windowSize, defaultZero),
          convertVoltsToMPSI(convertVoltsToMPSI) {}

    // returns new zero
    float recalibrate(int sampleCount) {
        std::vector<float> samples;
        for (int i = 0; i < sampleCount; i++) {
            samples.push_back(readVolts());
        }
        float newZero = utils::median(samples);
        setZero(newZero);
        return newZero;
    }

    void printSetZero() {
        Serial.print("Set zero to ");
        Serial.print(zero);
        Serial.print(" for ");
        Serial.println(debugName);
    }

    void setZeroToDefault() { setZero(defaultZero); }

    void setZero(float newZero) {
        zero = newZero;
        medianVolts.reset(newZero);
        printSetZero();
    }

    long getMPSI() {
        float volts = medianVolts.getLatest() - zero;
        return convertVoltsToMPSI(volts);
    }

    long getMedianMPSI() {
        float volts = medianVolts.getMedian() - zero;
        return convertVoltsToMPSI(volts);
    }

    float getRawVolts() { return medianVolts.getLatest(); }

    void init(uint8_t adc_addr, TwoWire &wire, adsGain_t gain) {
        printSetZero();  // print initial zero

        adc.setDataRate(RATE_ADS1115_860SPS);
        adc.setGain(gain);
        if (!adc.begin(adc_addr, &wire)) {
            Serial.print("Failed to initialize ADC for ");
            Serial.println(debugName);
            while (1)
                ;
        }
    }

    void tick() { medianVolts.add(readVolts()); }

   private:
    const char *debugName;
    const bool singleEnded;
    const bool flipVolts;
    const float defaultZero;
    const std::function<long(float)> convertVoltsToMPSI;

    Adafruit_ADS1115 adc;

    float zero;
    utils::MovingMedian<float> medianVolts;

    float readVolts() {
        float volts =
            adc.computeVolts(singleEnded ? adc.readADC_SingleEnded(0)
                                         : adc.readADC_Differential_0_1());
        if (flipVolts) {
            volts = -volts;
        }
        return volts;
    }
};

#endif  // MOVING_MEDIAN_ADC_H_