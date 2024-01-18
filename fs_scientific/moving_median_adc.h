#ifndef MOVING_MEDIAN_ADC_H_
#define MOVING_MEDIAN_ADC_H_

#include <Adafruit_ADS1X15.h>

#include "utils.h"

enum class ADCMode { SingleEnded, Differential };

class MovingMedianADC {
   public:
    MovingMedianADC(const char* debugName, int windowSize,
                    Adafruit_ADS1115& adc, ADCMode mode)
        : debugName(debugName),
          mode(mode),
          adc(adc),
          medianVolts(windowSize, 0) {}

    // returns new zero volts
    float recalibrate(int sampleCount) {
        std::vector<float> samples;
        for (int i = 0; i < sampleCount; i++) {
            samples.push_back(readVolts());
        }
        float newZeroVolts = utils::median(samples);
        setZero(newZeroVolts);
        return newZeroVolts;
    }

    void resetZero() { setZero(0); }

    void setZero(float newZeroVolts) {
        zeroVolts = newZeroVolts;
        medianVolts.reset(newZeroVolts);
        printSetZeroVolts();
    }

    float getLatestVolts() { return medianVolts.getLatest() - zeroVolts; }

    float getMedianVolts() { return medianVolts.getMedian() - zeroVolts; }

    // polls ADC for a new reading and saves it
    void tick() { medianVolts.add(readVolts()); }

   private:
    const char* debugName;
    Adafruit_ADS1115& adc;
    const ADCMode mode;

    float zeroVolts = 0;
    utils::MovingMedian<float> medianVolts;

    float readVolts() {
        return adc.computeVolts(mode == ADCMode::SingleEnded
                                    ? adc.readADC_SingleEnded(0)
                                    : adc.readADC_Differential_0_1());
    }

    void printSetZeroVolts() {
        Serial.print("Set zero to ");
        Serial.print(zeroVolts);
        Serial.print("volts for ");
        Serial.println(debugName);
    }
};

#endif  // MOVING_MEDIAN_ADC_H_