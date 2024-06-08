#include "hardware.h"

#include <Arduino.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <TickTwo.h>

#include "Adafruit_MAX31855.h"
#include "frequency_logger.h"
#include "sentence_serial.h"

namespace hardware {

const int PC_BAUD = 115200;
const int HARDWARE_INTERVAL = 5;

int64_t calibrationTime = 0;

namespace transducer {

long transd1 = 0;
long transd2 = 0;

long getTransd1() { return transd1; }
long getTransd2() { return transd2; }

}  // namespace transducer

namespace sciSerial {

// remember to connect TX to RX and RX to TX
const int RX_PIN = 48;
const int TX_PIN = 47;

const char *RECALIBRATE_SENTENCE = "cal";
const char *CLEAR_CALIBRATION_SENTENCE = "clear cal";

const char *CALIBRATION_COMPLETE_SENTENCE = "calibrated";

void processCompletedSentence(const char *sentence) {
    // possibility 1:
    if (strcmp(sentence, CALIBRATION_COMPLETE_SENTENCE) == 0) {
        calibrationTime = esp_timer_get_time();

        Serial.println("Calibration complete");
        return;
    }

    // possibility 2:

    // sentence format: [t1] [t2]
    // ex: "123456 123456"

    long t1, t2;
    int result = sscanf(sentence, "%ld %ld", &t1, &t2);

    if (result == 2) {
        transducer::transd1 = t1;
        transducer::transd2 = t2;
        return;
    }

    // else: invalid sentence
    Serial.print("Invalid sentence from scientific module: ");
    Serial.println(sentence);
}

SentenceSerial serial(Serial1, processCompletedSentence);

void sendRecalibrateCommand() { serial.sendSentence(RECALIBRATE_SENTENCE); }

void sendClearCalibrationCommand() {
    serial.sendSentence(CLEAR_CALIBRATION_SENTENCE);
}

void init() { serial.init(RX_PIN, TX_PIN); }

void tick() { serial.tick(); }

}  // namespace sciSerial

namespace relay {

// hardware mappings

const int RELAY1_PIN = 5;
const int RELAY2_PIN = 6;
const int RELAY3_PIN = 7;
const int RELAY4_PIN = 15;
const int RELAY5_PIN = 16;
const int RELAY6_PIN = 17;

const int SERVO1_PIN = 18;
const int SERVO2_PIN = 8;
const int SERVO3_PIN = 9;

const int AUX_PIN_1 = 1;

// logical mappings

const int FILL_PIN = RELAY1_PIN;
const int VENT_PIN = RELAY2_PIN;
const int ABORT_PIN = RELAY3_PIN;
const int PYRO_CUTTER_PIN = RELAY4_PIN;
const int IGNITER_PIN = RELAY5_PIN;
const int P_VALVE_PIN = AUX_PIN_1;

const int FILL_SERVO_PIN = SERVO1_PIN;

// end mappings

// TODO may need to flip
const int SERVO_VALVE_CLOSED_POS = 0;
const int SERVO_VALVE_OPEN_POS = 90;

Servo fillServo;

bool fillOn = false;
bool ventOn = false;
bool abortOn = false;
bool pyroCutterOn = false;
bool igniterOn = false;
bool pValveOn = false;

bool fillServoClosed = false;  // starts open

bool getFill() { return fillOn; }
bool getVent() { return ventOn; }
bool getAbort() { return abortOn; }
bool getPyroCutter() { return pyroCutterOn; }
bool getIgniter() { return igniterOn; }
bool getPValve() { return pValveOn; }

bool getFillServoClosed() { return fillServoClosed; }

void setFill(bool on) { fillOn = on; }
void setVent(bool on) { ventOn = on; }
void setAbort(bool on) { abortOn = on; }
void setPyroCutter(bool on) { pyroCutterOn = on; }
void setIgniter(bool on) { igniterOn = on; }
void setPValve(bool on) { pValveOn = on; }

void setFillServoClosed(bool closed) { fillServoClosed = closed; }

void writeRelay(int pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }

void flush() {
    writeRelay(FILL_PIN, fillOn);
    writeRelay(VENT_PIN, ventOn);
    writeRelay(ABORT_PIN, abortOn);
    writeRelay(PYRO_CUTTER_PIN, pyroCutterOn);
    writeRelay(IGNITER_PIN, igniterOn);

    // write LOW to turn on p-valve, as this pin sends a signal to a flight
    // computer input pin that is pulled up by default
    writeRelay(P_VALVE_PIN, pValveOn);

    fillServo.write(fillServoClosed ? SERVO_VALVE_CLOSED_POS
                                    : SERVO_VALVE_OPEN_POS);
}

void init() {
    pinMode(FILL_PIN, OUTPUT);
    pinMode(VENT_PIN, OUTPUT);
    pinMode(ABORT_PIN, OUTPUT);
    pinMode(PYRO_CUTTER_PIN, OUTPUT);
    pinMode(IGNITER_PIN, OUTPUT);
    pinMode(P_VALVE_PIN, OUTPUT);

    fillServo.attach(FILL_SERVO_PIN);

    flush();
}

}  // namespace relay

int64_t getCalibrationTime() { return calibrationTime; }

FrequencyLogger frequencyLogger("hardware", 1000);

void updateHardware() {
    frequencyLogger.tick();
    relay::flush();
}

TickTwo hardwareTicker(updateHardware, HARDWARE_INTERVAL);

// Writes initial output values, initializes serial to computer and scientific
// module.
void init() {
    // flush initial relay values before all else
    relay::init();

    Serial.begin(PC_BAUD);              // init serial to computer
    while (!Serial && millis() < 500);  // wait up to 500ms for serial to
                                        // connect; needed for native USB

    sciSerial::init();

    hardwareTicker.start();
}

// Flushes output values.
void tick() {
    sciSerial::tick();  // update as fast as possible
    hardwareTicker.update();
}

}  // namespace hardware
