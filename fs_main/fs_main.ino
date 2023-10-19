#include <Arduino.h>
#include <ArduinoJson.h>
#include <TickTwo.h>

#include "constants.h"
#include "hardware.h"
#include "interface.h"
#include "network.h"
#include "state.h"

const int SYNC_WITH_NETWORK_INTERVAL = 50;

void setStateReqBody() {
    int64_t timestamp = esp_timer_get_time();

    StaticJsonDocument<1024> doc;

    doc["stationId"] = constants::STATION_ID;
    doc["source"] = constants::STATION_STATE_RECORD_SOURCE;
    doc["timestamp"] = timestamp;

    JsonObject data = doc.createNestedObject("data");

    data["stateByte"] = interface::getStateByte();
    data["relayStatusByte"] = interface::getRelayStatusByte();
    data["oxTankMPSI"] = hardware::oxTank::getMPSI();
    data["ccMPSI"] = hardware::combustionChamber::getMPSI();
    data["timeSinceBoot"] = timestamp,
    data["timeSinceCalibration"] = timestamp - hardware::getCalibrationTime();

    char body[1024];
    serializeJson(doc, body, sizeof(body));

    network::setStateReqBody(body);
}

void getCommandResBody() {
    char body[1024];
    if (!network::getCommandResBody(body, sizeof(body))) {
        return;
    }

    StaticJsonDocument<1024> doc;
    auto err = deserializeJson(doc, body);

    if (err != DeserializationError::Ok) {
        Serial.print("Command response deserialization failed: ");
        Serial.println(err.f_str());
        return;
    }

    // sample response: {"data":{"command":"keep"}}

    const char* command = doc["data"]["command"];
    if (command == nullptr) {
        // no command
        return;
    }

    std::string commandStr(command);

    using namespace state;

    if (commandStr == RECALIBRATE_COMMAND) {
        hardware::sciSerial::sendRecalibrateCommand();
    } else if (commandStr == CLEAR_CALIBRATION_COMMAND) {
        hardware::sciSerial::sendClearCalibrationCommand();
    } else if (commandStr == STANDBY_COMMAND) {
        state::setOpState(OpState::standby);
    } else if (commandStr == KEEP_COMMAND) {
        state::setOpState(OpState::keep);
    } else if (commandStr == FILL_COMMAND) {
        state::setOpState(OpState::fill);
    } else if (commandStr == PURGE_COMMAND) {
        state::setOpState(OpState::purge);
    } else if (commandStr == PULSE_FILL_A_COMMAND) {
        state::setOpState(OpState::pulseFillA);
    } else if (commandStr == PULSE_FILL_B_COMMAND) {
        state::setOpState(OpState::pulseFillB);
    } else if (commandStr == PULSE_FILL_C_COMMAND) {
        state::setOpState(OpState::pulseFillC);
    } else if (commandStr == PULSE_VENT_A_COMMAND) {
        state::setOpState(OpState::pulseVentA);
    } else if (commandStr == PULSE_VENT_B_COMMAND) {
        state::setOpState(OpState::pulseVentB);
    } else if (commandStr == PULSE_VENT_C_COMMAND) {
        state::setOpState(OpState::pulseVentC);
    } else if (commandStr == PULSE_PURGE_A_COMMAND) {
        state::setOpState(OpState::pulsePurgeA);
    } else if (commandStr == PULSE_PURGE_B_COMMAND) {
        state::setOpState(OpState::pulsePurgeB);
    } else if (commandStr == PULSE_PURGE_C_COMMAND) {
        state::setOpState(OpState::pulsePurgeC);
    } else if (commandStr == FIRE_COMMAND) {
        state::setOpState(OpState::fire);
    } else if (commandStr == FIRE_MANUAL_IGNITER_COMMAND) {
        state::setOpState(OpState::fireManualIgniter);
    } else if (commandStr == FIRE_MANUAL_VALVE_COMMAND) {
        state::setOpState(OpState::fireManualValve);
    } else if (commandStr == ABORT_COMMAND) {
        state::setOpState(OpState::abort);
    } else if (commandStr == CUSTOM_COMMAND) {
        // `|` is the ArduinoJson way of setting the default value
        int relayStatusByte = doc["data"]["relayStatusByte"] | -1;
        if (relayStatusByte == -1) {
            Serial.println("No relayStatusByte in custom command");
            return;
        }
        state::setOpStateToCustom(relayStatusByte);
    } else {
        Serial.print("Unknown command: ");
        Serial.println(command);
    }
}

void syncWithNetwork() {
    setStateReqBody();
    getCommandResBody();
}

TickTwo syncWithNetworkTicker(syncWithNetwork, SYNC_WITH_NETWORK_INTERVAL);

void setup() {
    // init order matters
    hardware::init();
    state::init();
    network::init();

    syncWithNetworkTicker.start();
}

void loop() {
    // tick order matters
    state::tick();
    hardware::tick();

    syncWithNetworkTicker.update();

    delay(5);
}
