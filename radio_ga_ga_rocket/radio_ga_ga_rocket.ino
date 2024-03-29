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

// everything besides fix will be zero until the first time the GPS gets a fix
RadioPacket packet = {
    .gps_ts_tail = 0,
    .gps_fix = false,
    .gps_fixquality = 0,
    .gps_satellites = 0,
    .gps_latitude_fixed = 0,
    .gps_longitude_fixed = 0,
    .gps_altitude = 0,
};

void printGpsTs() {
    Serial.print("GPS ts_tail: ");
    Serial.println(packet.gps_ts_tail);
}

TickTwo printGpsTsTicker(printGpsTs, PRINT_GPS_TS_INTERVAL);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    spi.setPins(MISO, MOSI, SCK);

    if (!rf95.init()) Serial.println("init failed");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for
    // low power module) No encryption
    if (!rf95.setFrequency(FREQUENCY)) Serial.println("setFrequency failed");

    // If you are using a high power RF95 eg RFM95HW, you *must* set a Tx power
    // with the ishighpowermodule flag set like this:
    rf95.setTxPower(TX_POWER, false);

    // The encryption key has to be the same as the one in the server
    // uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    // 				  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    // 				};
    // rf95.setEncryptionKey(key);

    rockets_client::init(rockets_client::serverConfigPresets.ROCKET_PI, "0", "",
                         false, "GPS");

    printGpsTsTicker.start();
}

void loop() {
    rockets_client::StaticJsonDoc records = rockets_client::getLatestRecords();
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

    rf95.send((uint8_t*)&packet, sizeof(packet));
    rf95.waitPacketSent();

    printGpsTsTicker.update();

    // Serial.println("Sending to rf95_server");
    // // Send a message to rf95_server
    // uint8_t data[] = "Hello World!";
    // rf95.send(data, sizeof(data));

    // rf95.waitPacketSent();
    // // Now wait for a reply
    // uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    // uint8_t len = sizeof(buf);

    // if (rf95.waitAvailableTimeout(500)) {
    //     // Should be a reply message for us now
    //     if (rf95.recv(buf, &len)) {
    //         Serial.print("got reply: ");
    //         Serial.println((char*)buf);
    //     } else {
    //         Serial.println("recv failed");
    //     }
    // } else {
    //     Serial.println("No reply, is rf95_server running?");
    // }
    // delay(400);
}
