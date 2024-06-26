#include <RHSoftwareSPI.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <rockets_client.h>

#include "radio_packet.h"

#define SCK 16
#define MISO 7
#define MOSI 6
#define SS 5
#define INT 1

#define FREQUENCY 915.0
#define TX_POWER 23

#define UPLOAD_TO_SERVER true

// SPI with custom pins
RHSoftwareSPI spi;
// Singleton instance of the radio driver
RH_RF95 rf95(SS, INT, spi);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 500);  // wait up to 500ms for serial to
                                        // connect; needed for native USB

    spi.setPins(MISO, MOSI, SCK);

    if (!rf95.init()) Serial.println("init failed");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for
    // low power module) No encryption
    if (!rf95.setFrequency(FREQUENCY)) Serial.println("setFrequency failed");

    // If you are using a high power RF95 eg RFM95HW, you *must* set a Tx power
    // with the ishighpowermodule flag set like this:
    rf95.setTxPower(TX_POWER, false);

    if (UPLOAD_TO_SERVER) {
        rockets_client::init(rockets_client::wifiConfigPresets.GROUND,
                             rockets_client::serverConfigPresets.MECHE, "0",
                             "RadioGround");
    }
}

void loop() {
    rf95.waitAvailable();

    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    // try to receive packet
    if (!rf95.recv(buf, &len)) {
        Serial.println("recv failed");
        return;
    }

    // check that the length of the received packet is correct
    if (len != sizeof(RadioPacket)) {
        Serial.println("invalid packet size");
        return;
    }

    RadioPacket* packet = (RadioPacket*)buf;

    // send data to server

    rockets_client::StaticJsonDoc recordData;
    JsonObject gps = recordData.createNestedObject("gps");

    gps["ts_tail"] = packet->gps_ts_tail;
    gps["fix"] = packet->gps_fix;
    if (packet->gps_fix) {
        gps["fixquality"] = packet->gps_fixquality;
        gps["satellites"] = packet->gps_satellites;
        gps["latitude_fixed"] = packet->gps_latitude_fixed;
        gps["longitude_fixed"] = packet->gps_longitude_fixed;
        gps["altitude"] = packet->gps_altitude;
    }

    if (UPLOAD_TO_SERVER) {
        rockets_client::queueRecord(recordData);
    }

    // print packet

    Serial.print("ts_tail: ");
    Serial.print(packet->gps_ts_tail);

    Serial.print("\tfix: ");
    Serial.print(packet->gps_fix);

    if (packet->gps_fix) {
        Serial.print("\tfixquality: ");
        Serial.print(packet->gps_fixquality);

        Serial.print("\tsatellites: ");
        Serial.print(packet->gps_satellites);

        Serial.print("\tlatitude_fixed: ");
        Serial.print(packet->gps_latitude_fixed);

        Serial.print("\tlongitude_fixed: ");
        Serial.print(packet->gps_longitude_fixed);

        Serial.print("\taltitude: ");
        Serial.print(packet->gps_altitude);
    }

    Serial.println();
}
