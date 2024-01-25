#include "hardware.h"

#include <Arduino.h>
#include <ESP32Servo.h>
#include <TickTwo.h>

#include "frequency_logger.h"
#include "sentence_serial.h"

namespace hardware {

const int PC_BAUD = 115200;
const int HARDWARE_INTERVAL = 5;

int64_t calibrationTime = 0;

namespace oxTank {

long mpsi = 0;

long getMPSI() { return mpsi; }

}  // namespace oxTank

namespace combustionChamber {

long mpsi = 0;

long getMPSI() { return mpsi; }

}  // namespace combustionChamber

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

    // sentence format: "[ox_tank_pressure_mpsi_long] [cc_pressure_mpsi_long]"
    // ex: "123456 123456"

    // oxTank::mpsi = 0;
    // combustionChamber::mpsi = 0;

    long oxTankVal, ccVal;
    int result = sscanf(sentence, "%ld %ld", &oxTankVal, &ccVal);

    if (result == 2) {
        oxTank::mpsi = oxTankVal;
        combustionChamber::mpsi = ccVal;
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

// logical mappings

const int FILL_PIN = RELAY1_PIN;
const int VENT_PIN = RELAY2_PIN;
const int IGNITER_PIN = RELAY3_PIN;
const int PYRO_CUTTER_PIN = RELAY6_PIN;

const int SERVO_VALVE_PIN = SERVO1_PIN;

// end mappings

const int SERVO_VALVE_CLOSED_POS = 58;
const int SERVO_VALVE_OPEN_POS = 162;

// the valve is controlled by a servo rather than a relay
Servo servoValve;

bool fillOn = false;
bool ventOn = false;
bool pyroValveOn = false;
bool pyroCutterOn = false;
bool igniterOn = false;

bool servoValveAttached = false;

bool getFill() { return fillOn; }
bool getVent() { return ventOn; }
bool getPyroValve() { return pyroValveOn; }
bool getPyroCutter() { return pyroCutterOn; }
bool getIgniter() { return igniterOn; }

void setFill(bool on) { fillOn = on; }
void setVent(bool on) { ventOn = on; }
void setPyroValve(bool on) { pyroValveOn = on; }
void setPyroCutter(bool on) { pyroCutterOn = on; }
void setIgniter(bool on) { igniterOn = on; }

void setServoValveAttached(bool attached) { servoValveAttached = attached; }

void writeRelay(int pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }

void writeServoValveAttached(bool shouldAttach) {
    static bool _currentlyAttached = false;

    if (_currentlyAttached == shouldAttach) return;
    _currentlyAttached = shouldAttach;

    if (shouldAttach) {
        servoValve.attach(SERVO_VALVE_PIN);
    } else {
        servoValve.detach();
    }
}

void flush() {
    writeRelay(FILL_PIN, fillOn);
    writeRelay(VENT_PIN, ventOn);
    writeRelay(PYRO_CUTTER_PIN, pyroCutterOn);
    writeRelay(IGNITER_PIN, igniterOn);

    writeServoValveAttached(servoValveAttached);

    if (servoValveAttached) {
        servoValve.write(pyroValveOn ? SERVO_VALVE_OPEN_POS
                                     : SERVO_VALVE_CLOSED_POS);
    }
}

void init() {
    pinMode(FILL_PIN, OUTPUT);
    pinMode(VENT_PIN, OUTPUT);
    pinMode(PYRO_CUTTER_PIN, OUTPUT);
    pinMode(IGNITER_PIN, OUTPUT);

    // this is necessary to ensure the servo signal starts with 0V
    servoValve.attach(SERVO_VALVE_PIN);
    servoValve.detach();

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

    Serial.begin(PC_BAUD);  // init serial to computer
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    sciSerial::init();

    hardwareTicker.start();
}

// Flushes output values.
void tick() {
    sciSerial::tick();  // update as fast as possible
    hardwareTicker.update();
}

}  // namespace hardware
