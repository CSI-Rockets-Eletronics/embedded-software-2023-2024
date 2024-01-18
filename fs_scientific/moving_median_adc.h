#ifndef MOVING_MEDIAN_ADC_H_
#define MOVING_MEDIAN_ADC_H_

#include <Adafruit_ADS1X15.h>

#include "utils.h"

enum class ADCMode {
    SingleEnded_0,
    SingleEnded_1,
    SingleEnded_2,
    SingleEnded_3,
    Differential_0_1,
    Differential_2_3
};

template <typename ADCType>
class MovingMedianADC {
   public:
    MovingMedianADC(const char* debugName, int windowSize, ADCType& adc,
                    ADCMode mode)
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
    ADCType& adc;
    const ADCMode mode;

    float zeroVolts = 0;
    utils::MovingMedian<float> medianVolts;

    float readVolts() {
        int16_t counts = 0;

        switch (mode) {
            case ADCMode::SingleEnded_0:
                counts = adc.readADC_SingleEnded(0);
                break;
            case ADCMode::SingleEnded_1:
                counts = adc.readADC_SingleEnded(1);
                break;
            case ADCMode::SingleEnded_2:
                counts = adc.readADC_SingleEnded(2);
                break;
            case ADCMode::SingleEnded_3:
                counts = adc.readADC_SingleEnded(3);
                break;
            case ADCMode::Differential_0_1:
                counts = adc.readADC_Differential_0_1();
                break;
            case ADCMode::Differential_2_3:
                counts = adc.readADC_Differential_2_3();
                break;
        }

        return adc.computeVolts(counts);
    }

    void printSetZeroVolts() {
        Serial.print("Set zero to ");
        Serial.print(zeroVolts);
        Serial.print("volts for ");
        Serial.println(debugName);
    }
};

#endif  // MOVING_MEDIAN_ADC_H_