#include <EEPROM.h>
#include <TickTwo.h>
#include <Wire.h>
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
const int BIG_TRANSD_1_ZERO_EEPROM_ADDR = 4;  // store float
const int BIG_TRANSD_2_ZERO_EEPROM_ADDR = 8;  // store float

// I2C bus constants

const int SDA_PIN = 14;
const int SCL_PIN = 15;

// shared ADC constants

const int ADC_CALIBRATE_SAMPLE_COUNT = 500;
const int ADC_MEDIAN_WINDOW_SIZE = 50;

// specific ADC constants

const uint8_t ADC1_ADDR = 0x48;
const uint8_t ADC2_ADDR = 0x49;

// TODO set higher?
const uint16_t ADC1_RATE = RATE_ADS1015_1600SPS;
const uint16_t ADC2_RATE = RATE_ADS1015_1600SPS;

const adsGain_t ADC1_GAIN = GAIN_ONE;
const adsGain_t ADC2_GAIN = GAIN_ONE;

// device constants

const ADCMode BIG_TRANSD_1_ADC_MODE = ADCMode::SingleEnded_0;
const ADCMode BIG_TRANSD_2_ADC_MODE = ADCMode::SingleEnded_0;

const float BIG_TRANSD_1_MPSI_PER_VOLT = 1000000 / -1.202;
const float BIG_TRANSD_2_MPSI_PER_VOLT = 1000000;  // this is calibrated

// globals

TwoWire wire(0);

Adafruit_ADS1015 adc1;
Adafruit_ADS1015 adc2;

MovingMedianADC<Adafruit_ADS1015> bigTransd1ADC("big transducer 1",
                                                ADC_MEDIAN_WINDOW_SIZE, adc1,
                                                BIG_TRANSD_1_ADC_MODE);
MovingMedianADC<Adafruit_ADS1015> bigTransd2ADC("big transducer 2",
                                                ADC_MEDIAN_WINDOW_SIZE, adc2,
                                                BIG_TRANSD_2_ADC_MODE);

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

    int64_t ts_host = esp_timer_get_time();

    // DB should store raw readings, not median
    int32_t bt1_host =
        bigTransd1ADC.getLatestVolts() * BIG_TRANSD_1_MPSI_PER_VOLT;
    int32_t bt2_host =
        bigTransd2ADC.getLatestVolts() * BIG_TRANSD_2_MPSI_PER_VOLT;

    // deal with endianness
    uint64_t ts = htobe64(ts_host);
    uint32_t bt1 = htonl(bt1_host);
    uint32_t bt2 = htonl(bt2_host);

    serial.write((uint8_t *)&ts, sizeof(ts));    // 8 bytes
    serial.write((uint8_t *)&bt1, sizeof(bt1));  // 4 bytes
    serial.write((uint8_t *)&bt2, sizeof(bt2));  // 4 bytes

    serial.write(PACKET_DELIMITER, sizeof(PACKET_DELIMITER));

    if (PRINT_DEBUG) {
        Serial.print("bt1: ");
        Serial.print(bt1_host);
        Serial.print("\tbt2: ");
        Serial.println(bt2_host);
    }
}

}  // namespace piSerial

void readCalibration() {
    uint32_t markerVal = EEPROM.readUInt(0);
    if (markerVal == EEPROM_WRITTEN_MARKER) {
        bigTransd1ADC.setZero(EEPROM.readFloat(BIG_TRANSD_1_ZERO_EEPROM_ADDR));
        bigTransd2ADC.setZero(EEPROM.readFloat(BIG_TRANSD_2_ZERO_EEPROM_ADDR));
    }
}

void recalibrate() {
    float bt1Zero = bigTransd1ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);
    float bt2Zero = bigTransd2ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);

    EEPROM.writeUInt(0, EEPROM_WRITTEN_MARKER);
    EEPROM.writeFloat(BIG_TRANSD_1_ZERO_EEPROM_ADDR, bt1Zero);
    EEPROM.writeFloat(BIG_TRANSD_2_ZERO_EEPROM_ADDR, bt2Zero);
    EEPROM.commit();

    Serial.println("Recalibrated and saved to EEPROM");
}

void clearCalibration() {
    bigTransd1ADC.resetZero();
    bigTransd2ADC.resetZero();

    EEPROM.writeUInt(0, 0x00000000);
    EEPROM.commit();

    Serial.println("Cleared calibration from memory and EEPROM");
}

FrequencyLogger frequencyLogger("loop", 1000);

void setup() {
    Serial.begin(PC_BAUD);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    wire.begin(SDA_PIN, SCL_PIN);

    adc1.setDataRate(ADC1_RATE);
    adc1.setGain(ADC1_GAIN);
    if (!adc1.begin(ADC1_ADDR, &wire)) {
        Serial.println("Failed to start ADC1");
        while (1)
            ;
    }

    adc2.setDataRate(ADC2_RATE);
    adc2.setGain(ADC2_GAIN);
    if (!adc2.begin(ADC2_ADDR, &wire)) {
        Serial.println("Failed to start ADC2");
        while (1)
            ;
    }

    bigTransd1ADC.enableContinuous();
    bigTransd2ADC.enableContinuous();

    EEPROM.begin(EEPROM_SIZE);
    readCalibration();

    piSerial::init();
}

void loop() {
    frequencyLogger.tick();

    // read the ADCs and send sentences to the raspberry pi every tick
    bigTransd1ADC.tick();
    bigTransd2ADC.tick();

    piSerial::tick();
}
