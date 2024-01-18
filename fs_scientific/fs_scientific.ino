#include <EEPROM.h>
#include <TickTwo.h>

#include "frequency_logger.h"
#include "moving_median_adc.h"
#include "sentence_serial.h"

const bool PRINT_DEBUG = false;

// serial constants

const int PC_BAUD = 115200;
const int PI_BAUD = 115200;

// EEPROM constants

const int EEPROM_SIZE = 256;
// random int stored at address 0 that marks that subsequent values have been,
// avoid using 0x00000000 or 0xFFFFFFFF as they may be default values
const uint32_t EEPROM_WRITTEN_MARKER = 0x12345678;
const int OX_TANK_ZERO_EEPROM_ADDR = 4;  // store float
const int CC_ZERO_EEPROM_ADDR = 8;       // store float

// shared ADC constants

const int ADC_CALIBRATE_SAMPLE_COUNT = 100;
const int ADC_MEDIAN_WINDOW_SIZE = 5;

// specific ADC constants

const uint8_t ADC1_ADDR = 0x48;
const uint8_t ADC2_ADDR = 0x49;

const uint16_t ADC1_RATE = RATE_ADS1115_860SPS;
const uint16_t ADC2_RATE = RATE_ADS1115_860SPS;

const adsGain_t ADC1_GAIN = GAIN_ONE;
const adsGain_t ADC2_GAIN = GAIN_ONE;

// device constants

const ADCMode OX_TANK_ADC_MODE = ADCMode::SingleEnded;
const ADCMode CC_ADC_MODE = ADCMode::Differential;

const float OX_TANK_MPSI_PER_VOLT = 1000 / 0.00341944869;
const float CC_MPSI_PER_VOLT = 1000000 / 0.202;

// globals

Adafruit_ADS1115 adc1;
Adafruit_ADS1115 adc2;

MovingMedianADC<Adafruit_ADS1115> oxTankADC("ox tank", ADC_MEDIAN_WINDOW_SIZE,
                                            adc1, OX_TANK_ADC_MODE);
MovingMedianADC<Adafruit_ADS1115> ccADC("cc", ADC_MEDIAN_WINDOW_SIZE, adc2,
                                        CC_ADC_MODE);

void recalibrate();
void clearCalibration();

// to raspberry pi
namespace piSerial {

// remember to connect TX to RX and RX to TX
const int RX_PIN = 14;
const int TX_PIN = 13;

void init() { Serial2.begin(PI_BAUD, SERIAL_8N1, RX_PIN, TX_PIN); }

void tick() {
    // raspberry pi expects each line to be a JSON of a record's data field
    // ex: {"ox":123456,"cc":123456}

    // DB should store raw readings, not median
    long ox = oxTankADC.getLatestVolts() * OX_TANK_MPSI_PER_VOLT;
    long cc = ccADC.getLatestVolts() * CC_MPSI_PER_VOLT;

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
        recalibrate();
    } else if (sentenceStr == CLEAR_CALIBRATION_SENTENCE) {
        clearCalibration();
    } else {
        Serial.print("Invalid sentence from main module: ");
        Serial.println(sentence);
    }
}

SentenceSerial serial(Serial1, processCompletedSentence);

void sendSentence() {
    // sentence format: <ox_tank_pressure_mpsi_long cc_pressure_mpsi_long>
    // ex: <123456 123456>

    // send median readings to main board, as this gets displayed in the live UI
    long ox = oxTankADC.getMedianVolts() * OX_TANK_MPSI_PER_VOLT;
    long cc = ccADC.getMedianVolts() * CC_MPSI_PER_VOLT;

    char sentence[64];
    snprintf(sentence, sizeof(sentence), "%ld %ld", ox, cc);

    serial.sendSentence(sentence);

    if (PRINT_DEBUG) {
        Serial.print("Wrote sentence to main module: ");
        Serial.println(sentence);
    }
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
        oxTankADC.setZero(EEPROM.readFloat(OX_TANK_ZERO_EEPROM_ADDR));
        ccADC.setZero(EEPROM.readFloat(CC_ZERO_EEPROM_ADDR));
    }
}

void recalibrate() {
    float oxTankZero = oxTankADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);
    float ccZero = ccADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);

    EEPROM.writeUInt(0, EEPROM_WRITTEN_MARKER);
    EEPROM.writeFloat(OX_TANK_ZERO_EEPROM_ADDR, oxTankZero);
    EEPROM.writeFloat(CC_ZERO_EEPROM_ADDR, ccZero);
    EEPROM.commit();

    mainSerial::serial.sendSentence(mainSerial::CALIBRATION_COMPLETE_SENTENCE);

    Serial.println("Recalibrated and saved to EEPROM");
}

void clearCalibration() {
    oxTankADC.resetZero();
    ccADC.resetZero();

    EEPROM.writeUInt(0, 0x00000000);
    EEPROM.commit();

    Serial.println("Cleared calibration from memory and EEPROM");
}

FrequencyLogger frequencyLogger("loop", 1000);

void setup() {
    Serial.begin(PC_BAUD);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    adc1.setDataRate(ADC1_RATE);
    adc1.setGain(ADC1_GAIN);
    if (!adc1.begin(ADC1_ADDR)) {
        Serial.println("Failed to start ADC1");
        while (1)
            ;
    }

    adc2.setDataRate(ADC2_RATE);
    adc2.setGain(ADC2_GAIN);
    if (!adc2.begin(ADC2_ADDR)) {
        Serial.println("Failed to start ADC2");
        while (1)
            ;
    }

    EEPROM.begin(EEPROM_SIZE);
    readCalibration();

    piSerial::init();
    mainSerial::init();
}

void loop() {
    frequencyLogger.tick();

    // read the ADCs and send sentences to the raspberry pi every tick
    oxTankADC.tick();
    ccADC.tick();

    piSerial::tick();
    mainSerial::tick();
}
