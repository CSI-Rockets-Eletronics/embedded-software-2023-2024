#include <Arduino.h>
#include <TickTwo.h>
#include <rockets_client.h>

#include "hardware.h"
#include "interface.h"
#include "state.h"

const int SYNC_WITH_NETWORK_INTERVAL = 50;

void setStateReqBody() {
    int64_t timestamp = esp_timer_get_time();

    rockets_client::StaticJsonDoc recordData;

    recordData["stateByte"] = interface::getStateByte();
    recordData["relayStatusByte"] = interface::getRelayStatusByte();
    recordData["oxTankMPSI"] = hardware::oxTank::getMPSI();
    recordData["ccMPSI"] = hardware::combustionChamber::getMPSI();
    recordData["timeSinceBoot"] = timestamp,
    recordData["timeSinceCalibration"] =
        timestamp - hardware::getCalibrationTime();

    rockets_client::queueRecord(recordData);
}

void getCommandResBody() {
    rockets_client::StaticJsonDoc latestMessage =
        rockets_client::getLatestMessage();

    auto messageData = latestMessage["data"];
    if (messageData.isNull()) {
        // no new message
        return;
    }

    // sample response: {"data":{"command":"keep"}}

    const char* command = messageData["command"];
    if (command == NULL) {
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
        int relayStatusByte = messageData["relayStatusByte"] | -1;
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
    rockets_client::init(rockets_client::serverConfigPresets.FS_PI, "0",
                         "FiringStation", true);

    syncWithNetworkTicker.start();
}

void loop() {
    // tick order matters
    state::tick();
    hardware::tick();

    syncWithNetworkTicker.update();

    delay(5);
}
