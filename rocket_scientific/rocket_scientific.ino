#include <EEPROM.h>
#include <TickTwo.h>

#include "frequency_logger.h"
#include "moving_median_adc.h"
#include "sentence_serial.h"

const bool PRINT_DEBUG = true;

// serial constants

const int PC_BAUD = 115200;
const int PI_BAUD = 115200;

// EEPROM constants

const int EEPROM_SIZE = 256;
// random int stored at address 0 that marks that subsequent values have been,
// avoid using 0x00000000 or 0xFFFFFFFF as they may be default values
const uint32_t EEPROM_WRITTEN_MARKER = 0x12345678;
const int SMALL_TRANSD_1_ZERO_EEPROM_ADDR = 4;  // store float
const int SMALL_TRANSD_2_ZERO_EEPROM_ADDR = 8;  // store float

// shared ADC constants

const int ADC_CALIBRATE_SAMPLE_COUNT = 100;
const int ADC_MEDIAN_WINDOW_SIZE = 5;

// specific ADC constants

const uint8_t ADC1_ADDR = 0x48;
const uint8_t ADC2_ADDR = 0x49;

const uint16_t ADC1_RATE = RATE_ADS1115_860SPS;
const uint16_t ADC2_RATE = RATE_ADS1115_860SPS;

const adsGain_t ADC1_GAIN = GAIN_TWOTHIRDS;
const adsGain_t ADC2_GAIN = GAIN_TWOTHIRDS;

// device constants

const ADCMode SMALL_TRANSD_1_ADC_MODE = ADCMode::SingleEnded_0;
const ADCMode SMALL_TRANSD_2_ADC_MODE = ADCMode::SingleEnded_0;

const float SMALL_TRANSD_1_MPSI_PER_VOLT = 1000000;
const float SMALL_TRANSD_2_MPSI_PER_VOLT = 1000000;

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
const int RX_PIN = 18;
const int TX_PIN = 17;

// worst case, the delimiter occurs in the data, in which case we drop a packet
uint8_t PACKET_DELIMITER[] = {0b10101010, 0b01010101};

const char *RECALIBRATE_SENTENCE = "cal";
const char *CLEAR_CALIBRATION_SENTENCE = "clear cal";

void processCompletedSentence(const char *sentence) {
    std::string sentenceStr = sentence;

    if (sentenceStr == RECALIBRATE_SENTENCE) {
        recalibrate();
    } else if (sentenceStr == CLEAR_CALIBRATION_SENTENCE) {
        clearCalibration();
    } else {
        Serial.print("Invalid sentence from pi serial: ");
        Serial.println(sentence);
    }
}

SentenceSerial serial(Serial2, processCompletedSentence);

void init() { serial.init(RX_PIN, TX_PIN, PI_BAUD); }

void tick() {
    serial.tick();

    // DB should store raw readings, not median
    int32_t st1_host =
        smallTransd1ADC.getLatestVolts() * SMALL_TRANSD_1_MPSI_PER_VOLT;
    int32_t st2_host =
        smallTransd2ADC.getLatestVolts() * SMALL_TRANSD_2_MPSI_PER_VOLT;

    // deal with endianness
    uint32_t st1 = htonl(st1_host);
    uint32_t st2 = htonl(st2_host);

    serial.write((uint8_t *)&st1, sizeof(st1));  // 4 bytes
    serial.write((uint8_t *)&st2, sizeof(st2));  // 4 bytes

    serial.write(PACKET_DELIMITER, sizeof(PACKET_DELIMITER));

    if (PRINT_DEBUG) {
        Serial.print("st1: ");
        Serial.print(st1_host);
        Serial.print("\tst2: ");
        Serial.println(st2_host);
    }
}

}  // namespace piSerial

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
}

void loop() {
    frequencyLogger.tick();

    // read the ADCs and send sentences to the raspberry pi every tick
    smallTransd1ADC.tick();
    smallTransd2ADC.tick();

    piSerial::tick();
}
