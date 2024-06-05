#include <RHSoftwareSPI.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <TickTwo.h>
#include <rockets_client.h>

#include "radio_packet.h"

#define SCK 16
#define MISO 7
#define MOSI 6
#define SS 5
#define INT 1

#define FREQUENCY 915.0
#define TX_POWER 23

#define PRINT_GPS_TS_INTERVAL 1000

// SPI with custom pins
RHSoftwareSPI spi;
// Singleton instance of the radio driver
RH_RF95 rf95(SS, INT, spi);

RadioPacket packet;

void printGpsTs() {
    Serial.print("GPS ts_tail: ");
    Serial.println(packet.gps_ts_tail);
}

TickTwo printGpsTsTicker(printGpsTs, PRINT_GPS_TS_INTERVAL);

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

    rockets_client::init(rockets_client::wifiConfigPresets.ROCKET,
                         rockets_client::serverConfigPresets.ROCKET_PI, "0", "",
                         false, "GPS,Trajectory,RocketScientific");

    printGpsTsTicker.start();
}

void loop() {
    // zero out unset fields
    memset(&packet, 0, sizeof(packet));

    // ===== get records =====

    rockets_client::StaticJsonDoc records = rockets_client::getLatestRecords();

    // if any data is missing, we'll just let the JSON parsing crash
    // and reboot lol...

    // ===== parse GPS =====

    JsonObject gps = records["GPS"]["data"];

    packet.gps_ts_tail = gps["ts_tail"];
    packet.gps_fix = gps["fix"];

    if (packet.gps_fix) {
        packet.gps_fixquality = gps["fixquality"];
        packet.gps_satellites = gps["satellites"];
        packet.gps_latitude_fixed = gps["latitude_fixed"];
        packet.gps_longitude_fixed = gps["longitude_fixed"];
        packet.gps_altitude = gps["altitude"];
    }

    // ===== parse Trajectory =====

    JsonObject trajectory = records["Trajectory"]["data"];

    packet.trajectory_z = trajectory["z"];
    packet.trajectory_vz = trajectory["vz"];
    packet.trajectory_az = trajectory["az"];

    // ===== parse RocketScientific =====

    JsonObject rocketScientific = records["RocketScientific"]["data"];

    packet.rocket_scientific_transducer1 = rocketScientific["transducer1"];
    packet.rocket_scientific_transducer3 = rocketScientific["transducer3"];

    // ==== send packet =====

    rf95.send((uint8_t*)&packet, sizeof(packet));
    rf95.waitPacketSent();

    printGpsTsTicker.update();
}
