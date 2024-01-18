#include "hardware.h"

#include <EEPROM.h>
#include <TickTwo.h>

#include "frequency_logger.h"
#include "moving_median_adc.h"
#include "sentence_serial.h"

namespace hardware {

const int PC_BAUD = 115200;
const int PI_BAUD = 115200;

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
                    convertVoltsToMPSI, false);

}  // namespace combustionChamber

// to raspberry pi
namespace piSerial {

// remember to connect TX to RX and RX to TX
const int RX_PIN = 14;
const int TX_PIN = 13;

void init() { Serial2.begin(PI_BAUD, SERIAL_8N1, RX_PIN, TX_PIN); }

void tick() {
    // raspberry pi expects each line to be a JSON of a record's data field
    // ex: {"ox":123456,"cc":123456}

    long ox = oxTank::adc.getMPSI();
    long cc = combustionChamber::adc.getMPSI();

    char sentence[64];
    snprintf(sentence, sizeof(sentence), "{\"ox\":%ld,\"cc\":%ld}", ox, cc);

    Serial2.println(sentence);
}

}  // namespace piSerial

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
        Serial.print("Invalid sentence from main module: ");
        Serial.println(sentence);
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

    // Serial.print("Wrote sentence to main module: ");
    // Serial.println(sentence);
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

    Serial.println("Recalibrated and saved to EEPROM");
}

void clearCalibration() {
    oxTank::adc.setZeroToDefault();
    combustionChamber::adc.setZeroToDefault();

    EEPROM.writeUInt(0, 0x00000000);
    EEPROM.commit();

    Serial.println("Cleared calibration from memory and EEPROM");
}

FrequencyLogger frequencyLogger("hardware", 1000);

void init() {
    Serial.begin(PC_BAUD);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    oxTank::adc.init(OX_TANK_ADC_ADDR, Wire, OX_TANK_ADC_GAIN);
    combustionChamber::adc.init(CC_ADC_ADDR, Wire, CC_ADC_GAIN);

    EEPROM.begin(EEPROM_SIZE);
    readCalibration();

    piSerial::init();
    mainSerial::init();
}

void tick() {
    frequencyLogger.tick();

    // read the ADCs and send sentences to the raspberry pi every tick
    oxTank::adc.tick();
    combustionChamber::adc.tick();
    piSerial::tick();

    mainSerial::tick();
}

}  // namespace hardware
