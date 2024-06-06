#include <RHSoftwareSPI.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <rockets_client.h>

#include "radio_packet.h"

// ! uncomment below to switch between 433Mhz and 915Mhz modules
#define USE_433MHZ_MODULE
// #define USE_915MHZ_MODULE

#ifdef USE_433MHZ_MODULE

#define SCLK 12
#define MISO 13
#define MOSI 11
#define SS 10
#define INT 2

#define FREQUENCY 433.0

#endif  // USE_433MHZ_MODULE

#ifdef USE_915MHZ_MODULE

#define SCK 16
#define MISO 7
#define MOSI 6
#define SS 5
#define INT 1

#define FREQUENCY 915.0

#endif  // USE_915MHZ_MODULE

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
        rockets_client::init(rockets_client::wifiConfigPresets.GROUND_LONG,
                             rockets_client::serverConfigPresets.FS_PI, "0",
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

    // construct data object

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

    JsonObject trajectory = recordData.createNestedObject("trajectory");

    trajectory["z"] = packet->trajectory_z;
    trajectory["vz"] = packet->trajectory_vz;
    trajectory["az"] = packet->trajectory_az;

    JsonObject rocket_scientific =
        recordData.createNestedObject("rocketScientific");

    rocket_scientific["t1"] = packet->rocket_scientific_transducer1;
    rocket_scientific["t3"] = packet->rocket_scientific_transducer3;

    // send data to server

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

    Serial.print("z: ");
    Serial.print(packet->trajectory_z);

    Serial.print("\tvz: ");
    Serial.print(packet->trajectory_vz);

    Serial.print("\taz: ");
    Serial.print(packet->trajectory_az);

    Serial.println();

    Serial.print("transducer1: ");
    Serial.print(packet->rocket_scientific_transducer1);

    Serial.print("\ttransducer3: ");
    Serial.print(packet->rocket_scientific_transducer3);

    Serial.println();
    Serial.println();
}
