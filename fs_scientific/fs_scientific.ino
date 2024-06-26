#include <EEPROM.h>
#include <TickTwo.h>
#include <endian.h>

#include "frequency_logger.h"
#include "moving_median_adc.h"
#include "sentence_serial.h"

const bool PRINT_DEBUG = false;

// serial constants

const int PC_BAUD = 115200;
const int PI_BAUD = 230400;

// EEPROM constants

const int EEPROM_SIZE = 256;
// random int stored at address 0 that marks that subsequent values have been,
// avoid using 0x00000000 or 0xFFFFFFFF as they may be default values
const uint32_t EEPROM_WRITTEN_MARKER = 0x12345678;
const int SMALL_TRANSD_1_ZERO_EEPROM_ADDR = 4;  // store float
const int SMALL_TRANSD_2_ZERO_EEPROM_ADDR = 8;  // store float

// shared ADC constants

const int ADC_CALIBRATE_SAMPLE_COUNT = 500;
const int ADC_MEDIAN_WINDOW_SIZE = 50;

// specific ADC constants

const uint8_t ADC1_ADDR = 0x48;
const uint8_t ADC2_ADDR = 0x49;

const uint16_t ADC1_RATE = RATE_ADS1115_860SPS;
const uint16_t ADC2_RATE = RATE_ADS1115_860SPS;

const adsGain_t ADC1_GAIN = GAIN_TWOTHIRDS;
const adsGain_t ADC2_GAIN = GAIN_TWOTHIRDS;

// device constants

const ADCMode SMALL_TRANSD_1_ADC_MODE = ADCMode::SingleEnded_2;
const ADCMode SMALL_TRANSD_2_ADC_MODE = ADCMode::SingleEnded_2;

const float SMALL_TRANSD_1_MPSI_PER_VOLT = 1000 / 0.00341944869;
const float SMALL_TRANSD_2_MPSI_PER_VOLT = 1000 / 0.00341944869;

// globals

Adafruit_ADS1115 adc1;
Adafruit_ADS1115 adc2;

MovingMedianADC<Adafruit_ADS1115> smallTransd1ADC("small transducer 1",
                                                  ADC_MEDIAN_WINDOW_SIZE, adc1,
                                                  SMALL_TRANSD_1_ADC_MODE);
MovingMedianADC<Adafruit_ADS1115> smallTransd2ADC("small transducer 2",
                                                  ADC_MEDIAN_WINDOW_SIZE, adc2,
                                                  SMALL_TRANSD_2_ADC_MODE);

void recalibrate();
void clearCalibration();

// to raspberry pi
namespace piSerial {

// remember to connect TX to RX and RX to TX
const int RX_PIN = 14;
const int TX_PIN = 13;

// worst case, the delimiter occurs in the data, in which case we drop a packet
uint8_t PACKET_DELIMITER[] = {0b10101010, 0b01010101};

void init() { Serial2.begin(PI_BAUD, SERIAL_8N1, RX_PIN, TX_PIN); }

void tick() {
    int64_t ts_host = esp_timer_get_time();

    // DB should store raw readings, not median
    int32_t st1_host =
        smallTransd1ADC.getLatestVolts() * SMALL_TRANSD_1_MPSI_PER_VOLT;
    int32_t st2_host =
        smallTransd2ADC.getLatestVolts() * SMALL_TRANSD_2_MPSI_PER_VOLT;

    // deal with endianness
    uint64_t ts = htobe64(ts_host);
    uint32_t st1 = htonl(st1_host);
    uint32_t st2 = htonl(st2_host);

    Serial2.write((uint8_t *)&ts, sizeof(ts));    // 8 bytes
    Serial2.write((uint8_t *)&st1, sizeof(st1));  // 4 bytes
    Serial2.write((uint8_t *)&st2, sizeof(st2));  // 4 bytes

    Serial2.write(PACKET_DELIMITER, sizeof(PACKET_DELIMITER));
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
    // sentence format: <st1_pressure_mpsi_long st2_pressure_mpsi_long>
    // ex: <123456 123456>

    // send median readings to main board, as this gets displayed in the live UI
    long st1 = smallTransd1ADC.getMedianVolts() * SMALL_TRANSD_1_MPSI_PER_VOLT;
    long st2 = smallTransd2ADC.getMedianVolts() * SMALL_TRANSD_2_MPSI_PER_VOLT;

    char sentence[64];
    snprintf(sentence, sizeof(sentence), "%ld %ld", st1, st2);

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
        smallTransd1ADC.setZero(
            EEPROM.readFloat(SMALL_TRANSD_1_ZERO_EEPROM_ADDR));
        smallTransd2ADC.setZero(
            EEPROM.readFloat(SMALL_TRANSD_2_ZERO_EEPROM_ADDR));
    }
}

void recalibrate() {
    float st1Zero = smallTransd1ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);
    float st2Zero = smallTransd2ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);

    EEPROM.writeUInt(0, EEPROM_WRITTEN_MARKER);
    EEPROM.writeFloat(SMALL_TRANSD_1_ZERO_EEPROM_ADDR, st1Zero);
    EEPROM.writeFloat(SMALL_TRANSD_2_ZERO_EEPROM_ADDR, st2Zero);
    EEPROM.commit();

    mainSerial::serial.sendSentence(mainSerial::CALIBRATION_COMPLETE_SENTENCE);

    Serial.println("Recalibrated and saved to EEPROM");
}

void clearCalibration() {
    smallTransd1ADC.resetZero();
    smallTransd2ADC.resetZero();

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

    smallTransd1ADC.enableContinuous();
    smallTransd2ADC.enableContinuous();

    EEPROM.begin(EEPROM_SIZE);
    readCalibration();

    piSerial::init();
    mainSerial::init();
}

void loop() {
    frequencyLogger.tick();

    // read the ADCs and send sentences to the raspberry pi every tick
    smallTransd1ADC.tick();
    smallTransd2ADC.tick();

    piSerial::tick();
    mainSerial::tick();
}
