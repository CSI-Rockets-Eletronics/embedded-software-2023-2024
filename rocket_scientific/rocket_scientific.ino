#include <EEPROM.h>
#include <TickTwo.h>
#include <endian.h>

#include "frequency_logger.h"
#include "moving_median_adc.h"
#include "sentence_serial.h"
#include "serial_forwarder.h"

const bool PRINT_DEBUG = false;

// serial constants

const int PC_BAUD = 115200;
const int PI_BAUD = 230400;

// EEPROM constants

const int EEPROM_SIZE = 256;
// random int stored at address 0 that marks that subsequent values have been,
// avoid using 0x00000000 or 0xFFFFFFFF as they may be default values
const uint32_t EEPROM_WRITTEN_MARKER = 0x12345678;
const int TRANSD_1_ZERO_EEPROM_ADDR = 4;   // store float
const int TRANSD_2_ZERO_EEPROM_ADDR = 8;   // store float
const int TRANSD_3_ZERO_EEPROM_ADDR = 12;  // store float

// I2C constants

const int BUS1_SDA_PIN = 8;
const int BUS1_SCL_PIN = 9;

const int BUS2_SDA_PIN = 14;
const int BUS2_SCL_PIN = 15;

// shared ADC constants

const int ADC_CALIBRATE_SAMPLE_COUNT = 500;
const int ADC_MEDIAN_WINDOW_SIZE = 50;

// specific ADC constants

const uint8_t ADC1_ADDR = 0x48;
const uint8_t ADC2_ADDR = 0x49;
const uint8_t ADC3_ADDR = 0x48;

const uint16_t ADC1_RATE = RATE_ADS1115_860SPS;
const uint16_t ADC2_RATE = RATE_ADS1115_860SPS;
const uint16_t ADC3_RATE = RATE_ADS1115_860SPS;

const adsGain_t ADC1_GAIN = GAIN_ONE;
const adsGain_t ADC2_GAIN = GAIN_ONE;
const adsGain_t ADC3_GAIN = GAIN_ONE;

// device constants

const ADCMode TRANSD_1_ADC_MODE = ADCMode::SingleEnded_0;
const ADCMode TRANSD_2_ADC_MODE = ADCMode::SingleEnded_0;
const ADCMode TRANSD_3_ADC_MODE = ADCMode::SingleEnded_0;

const float TRANSD_1_MPSI_PER_VOLT = 1000 / 0.00341944869;
const float TRANSD_2_MPSI_PER_VOLT = 1000 / 0.00341944869;
const float TRANSD_3_MPSI_PER_VOLT = 1000000;

// globals

Adafruit_ADS1115 adc1;
Adafruit_ADS1115 adc2;
Adafruit_ADS1115 adc3;

MovingMedianADC<Adafruit_ADS1115> Transd1ADC("transducer 1",
                                             ADC_MEDIAN_WINDOW_SIZE, adc1,
                                             TRANSD_1_ADC_MODE);
MovingMedianADC<Adafruit_ADS1115> Transd2ADC("transducer 2",
                                             ADC_MEDIAN_WINDOW_SIZE, adc2,
                                             TRANSD_2_ADC_MODE);
MovingMedianADC<Adafruit_ADS1115> Transd3ADC("transducer 3",
                                             ADC_MEDIAN_WINDOW_SIZE, adc3,
                                             TRANSD_3_ADC_MODE);

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
    int32_t t1_host = Transd1ADC.getLatestVolts() * TRANSD_1_MPSI_PER_VOLT;
    int32_t t2_host = Transd2ADC.getLatestVolts() * TRANSD_2_MPSI_PER_VOLT;
    int32_t t3_host = Transd3ADC.getLatestVolts() * TRANSD_3_MPSI_PER_VOLT;

    // deal with endianness
    uint64_t ts = htobe64(ts_host);
    uint32_t t1 = htonl(t1_host);
    uint32_t t2 = htonl(t2_host);
    uint32_t t3 = htonl(t3_host);

    serial.write((uint8_t *)&ts, sizeof(ts));  // 8 bytes
    serial.write((uint8_t *)&t1, sizeof(t1));  // 4 bytes
    serial.write((uint8_t *)&t2, sizeof(t2));  // 4 bytes
    serial.write((uint8_t *)&t3, sizeof(t3));  // 4 bytes

    serial.write(PACKET_DELIMITER, sizeof(PACKET_DELIMITER));

    if (PRINT_DEBUG) {
        Serial.print("t1: ");
        Serial.print(t1_host);
        Serial.print("\tt2: ");
        Serial.println(t2_host);
        Serial.print("\tt3: ");
        Serial.println(t3_host);
    }

    int queuedPacketLen;
    uint8_t *queuedPacket = serialForwarder::getQueuedPacket(&queuedPacketLen);

    if (queuedPacketLen > 0) {
        serial.write(queuedPacket, queuedPacketLen);

        if (PRINT_DEBUG) {
            Serial.print("Forwarded packet to pi: length = ");
            Serial.println(queuedPacketLen);
        }
    }
}

}  // namespace piSerial

void readCalibration() {
    uint32_t markerVal = EEPROM.readUInt(0);
    if (markerVal == EEPROM_WRITTEN_MARKER) {
        Transd1ADC.setZero(EEPROM.readFloat(TRANSD_1_ZERO_EEPROM_ADDR));
        Transd2ADC.setZero(EEPROM.readFloat(TRANSD_2_ZERO_EEPROM_ADDR));
        Transd3ADC.setZero(EEPROM.readFloat(TRANSD_3_ZERO_EEPROM_ADDR));
    }
}

void recalibrate() {
    float t1Zero = Transd1ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);
    float t2Zero = Transd2ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);
    float t3Zero = Transd3ADC.recalibrate(ADC_CALIBRATE_SAMPLE_COUNT);

    EEPROM.writeUInt(0, EEPROM_WRITTEN_MARKER);
    EEPROM.writeFloat(TRANSD_1_ZERO_EEPROM_ADDR, t1Zero);
    EEPROM.writeFloat(TRANSD_2_ZERO_EEPROM_ADDR, t2Zero);
    EEPROM.writeFloat(TRANSD_3_ZERO_EEPROM_ADDR, t3Zero);
    EEPROM.commit();

    Serial.println("Recalibrated and saved to EEPROM");
}

void clearCalibration() {
    Transd1ADC.resetZero();
    Transd2ADC.resetZero();
    Transd3ADC.resetZero();

    EEPROM.writeUInt(0, 0x00000000);
    EEPROM.commit();

    Serial.println("Cleared calibration from memory and EEPROM");
}

FrequencyLogger frequencyLogger("loop", 1000);

void setup() {
    Serial.begin(PC_BAUD);
    while (!Serial && millis() < 500);  // wait up to 500ms for serial to
                                        // connect; needed for native USB

    Wire.begin(BUS1_SDA_PIN, BUS1_SCL_PIN);
    Wire1.begin(BUS2_SDA_PIN, BUS2_SCL_PIN);

    adc1.setDataRate(ADC1_RATE);
    adc1.setGain(ADC1_GAIN);
    if (!adc1.begin(ADC1_ADDR, &Wire)) {
        Serial.println("Failed to start ADC1");
        while (1);
    }

    adc2.setDataRate(ADC2_RATE);
    adc2.setGain(ADC2_GAIN);
    if (!adc2.begin(ADC2_ADDR, &Wire)) {
        Serial.println("Failed to start ADC2");
        while (1);
    }

    adc3.setDataRate(ADC3_RATE);
    adc3.setGain(ADC3_GAIN);
    if (!adc3.begin(ADC3_ADDR, &Wire1)) {
        Serial.println("Failed to start ADC3");
        while (1);
    }

    Transd1ADC.enableContinuous();
    Transd2ADC.enableContinuous();
    Transd3ADC.enableContinuous();

    EEPROM.begin(EEPROM_SIZE);
    readCalibration();

    piSerial::init();

    serialForwarder::init();
}

void loop() {
    frequencyLogger.tick();

    // read the ADCs and send sentences to the raspberry pi every tick
    Transd1ADC.tick();
    Transd2ADC.tick();
    Transd3ADC.tick();

    piSerial::tick();

    serialForwarder::tick();
}
