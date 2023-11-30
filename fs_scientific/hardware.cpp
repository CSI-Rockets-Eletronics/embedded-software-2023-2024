#include "hardware.h"

#include <Adafruit_ADS1X15.h>
#include <EEPROM.h>
#include <TickTwo.h>

#include <vector>

#include "frequency_logger.h"
#include "fsDataDriver.h"
#include "sentence_serial.h"
#include "utils.h"

namespace hardware {

// frequency for everything except serial to the main module
// (oh boy, 3kHz)
const int PRIMARY_FREQ_HZ = 3000;

const int ADC_CALIBRATE_SAMPLE_COUNT = 100;

const int OX_TANK_ADC_MEDIAN_WINDOW_SIZE = 5;
const int CC_ADC_MEDIAN_WINDOW_SIZE = 25;

const int EEPROM_SIZE = 256;
// random int stored at address 0 that marks that subsequent values have been,
// avoid using 0x00000000 or 0xFFFFFFFF as they may be default values
const uint32_t EEPROM_WRITTEN_MARKER = 0x12345678;
const int OX_TANK_ZERO_EEPROM_ADDR = 4;  // float
const int CC_ZERO_EEPROM_ADDR = 8;       // float

const uint8_t OX_TANK_ADC_ADDR = 0x48;
const uint8_t CC_ADC_ADDR = 0x49;

const adsGain_t OX_TANK_ADC_GAIN = GAIN_ONE;
const adsGain_t CC_ADC_GAIN = GAIN_EIGHT;

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
        usbSerial::debugPrint("Set zero to ");
        usbSerial::debugPrint(zero);
        usbSerial::debugPrint(" for ");
        usbSerial::debugPrintln(debugName);
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

        adc.setGain(gain);
        if (!adc.begin(adc_addr, &wire)) {
            usbSerial::debugPrint("Failed to initialize ADC for ");
            usbSerial::debugPrintln(debugName);
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

namespace oxTank {

const float DEFAULT_ZERO = 0.53;

long convertVoltsToMPSI(float volts) {
    return (long)(volts * 1000 / 0.00341944869);
}

MovingMedianADC adc("ox tank", OX_TANK_ADC_MEDIAN_WINDOW_SIZE, DEFAULT_ZERO,
                    convertVoltsToMPSI, true);

}  // namespace oxTank

namespace combustionChamber {

const float DEFAULT_ZERO = 0;

long convertVoltsToMPSI(float volts) { return (long)(volts * 1000000 / 0.202); }

MovingMedianADC adc("cc", CC_ADC_MEDIAN_WINDOW_SIZE, DEFAULT_ZERO,
                    convertVoltsToMPSI, false, true);

}  // namespace combustionChamber

// to raspberry pi or computer (for debugging)
namespace usbSerial {

const int PC_BAUD = 115200;
const int PI_BAUD = 115200;

void sendScientificPacket() {
    // don't send packet if connected to computer
    if (PRINT_DEBUG_TO_SERIAL) return;

    unsigned long time = micros();
    int timeSeconds = time / 1000000;

    fsDataDriver::ScientificDataPacket packet = {
        .time = time,
        .oxTankPressure = oxTank::adc.getMPSI(),
        .ccPressure = combustionChamber::adc.getMPSI(),
        .oxidizerTankTransducerValue = oxTank::adc.getRawVolts() * 1000000,
        .combustionChamberTransducerValue =
            combustionChamber::adc.getRawVolts() * 1000000,
        .timeSinceLastCalibration = 0,  // TODO
        .timeSinceLastStartup = max(timeSeconds, 255),
    };

    byte buf[fsDataDriver::SCIENTIFIC_DATA_PACKET_SIZE];
    packet.dumpPacket(buf);

    Serial.write(buf, sizeof(buf));
}

void init() {
    Serial.begin(PRINT_DEBUG_TO_SERIAL ? PC_BAUD : PI_BAUD);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB
}

}  // namespace usbSerial

// to main board
namespace mainSerial {

// remember to connect TX to RX and RX to TX
const int RX_PIN = 47;
const int TX_PIN = 48;

const int SENTENCE_INTERVAL = 50;

const char *RECALIBRATE_SENTENCE = "cal";
const char *CLEAR_CALIBRATION_SENTENCE = "clear cal";

const char *CALIBRATION_COMPLETE_SENTENCE = "calibrated";

void processCompletedSentence(const char *sentence) {
    std::string sentenceStr = sentence;

    if (sentenceStr == RECALIBRATE_SENTENCE) {
        hardware::recalibrate();
    } else if (sentenceStr == CLEAR_CALIBRATION_SENTENCE) {
        hardware::clearCalibration();
    } else {
        usbSerial::debugPrint("Invalid sentence from main module: ");
        usbSerial::debugPrintln(sentence);
    }
}

SentenceSerial serial(Serial1, processCompletedSentence);

void sendSentence() {
    // sentence format: <ox_tank_pressure_mpsi_long cc_pressure_mpsi_long>
    // ex: <123456 123456>

    char sentence[64];
    snprintf(sentence, sizeof(sentence), "%ld %ld", oxTank::adc.getMedianMPSI(),
             combustionChamber::adc.getMedianMPSI());

    serial.sendSentence(sentence);

    // usbSerial::debugPrint("Wrote sentence to main module: ");
    // usbSerial::debugPrintln(sentence);
}

TickTwo sentenceTicker(sendSentence, SENTENCE_INTERVAL);

void init() {
    serial.init(RX_PIN, TX_PIN);
    sentenceTicker.start();
}

void tick() {
    serial.tick();
    sentenceTicker.update();
}

}  // namespace mainSerial

void readCalibration() {
    uint32_t markerVal = EEPROM.readUInt(0);
    if (markerVal == EEPROM_WRITTEN_MARKER) {
        oxTank::adc.setZero(EEPROM.readFloat(OX_TANK_ZERO_EEPROM_ADDR));
        combustionChamber::adc.setZero(EEPROM.readFloat(CC_ZERO_EEPROM_ADDR));
    }
}

void recalibrate() {
    float oxTankZero = oxTank::adc.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);
    float ccZero =
        combustionChamber::adc.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);

    EEPROM.writeUInt(0, EEPROM_WRITTEN_MARKER);
    EEPROM.writeFloat(OX_TANK_ZERO_EEPROM_ADDR, oxTankZero);
    EEPROM.writeFloat(CC_ZERO_EEPROM_ADDR, ccZero);
    EEPROM.commit();

    mainSerial::serial.sendSentence(mainSerial::CALIBRATION_COMPLETE_SENTENCE);

    usbSerial::debugPrintln("Recalibrated and saved to EEPROM");
}

void clearCalibration() {
    oxTank::adc.setZeroToDefault();
    combustionChamber::adc.setZeroToDefault();

    EEPROM.writeUInt(0, 0x00000000);
    EEPROM.commit();

    usbSerial::debugPrintln("Cleared calibration from memory and EEPROM");
}

FrequencyLogger frequencyLogger("hardware", 1000);

void primaryUpdate() {
    frequencyLogger.tick();

    oxTank::adc.tick();
    combustionChamber::adc.tick();

    usbSerial::sendScientificPacket();
}

TickTwo primaryUpdateTicker(primaryUpdate, 1000000 / PRIMARY_FREQ_HZ, 0,
                            MICROS_MICROS);

void init() {
    usbSerial::init();

    oxTank::adc.init(OX_TANK_ADC_ADDR, Wire, OX_TANK_ADC_GAIN);
    combustionChamber::adc.init(CC_ADC_ADDR, Wire, CC_ADC_GAIN);

    EEPROM.begin(EEPROM_SIZE);
    readCalibration();

    mainSerial::init();

    primaryUpdateTicker.start();
}

void tick() {
    primaryUpdateTicker.update();

    mainSerial::tick();
}

}  // namespace hardware
