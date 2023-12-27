// rf95_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_server.
// Demonstrates the use of AES encryption, setting the frequency and modem
// configuration
// Tested on Moteino with RFM95 http://lowpowerlab.com/moteino/
// Tested on miniWireless with RFM95 www.anarduino.com/miniwireless
// Tested on Teensy 3.1 with RF95 on PJRC breakout board

#include <RH_RF95.h>
#include <SPI.h>
#include <rockets_client.h>

#include "radio_packet.h"

#define SCLK 12
#define MISO 13
#define MOSI 11
#define SS 10
#define INT 2

// Singleton instance of the radio driver
RH_RF95 rf95;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    if (!rf95.init()) Serial.println("init failed");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for
    // low power module) No encryption
    if (!rf95.setFrequency(434.0)) Serial.println("setFrequency failed");

    // If you are using a high power RF95 eg RFM95HW, you *must* set a Tx power
    // with the ishighpowermodule flag set like this:
    rf95.setTxPower(23, false);

    // The encryption key has to be the same as the one in the server
    // uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    // 				  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    // 				};
    // rf95.setEncryptionKey(key);

    rockets_client::init(rockets_client::serverConfigPresets.ROCKET_PI, "0", "",
                         false, "GPS");
}

// everything besides fix will be zero until the first time the GPS gets a fix
RadioPacket packet = {
    .fix = false,
    .fixquality = 0,
    .satellites = 0,
    .PDOP_10 = 0,
    .latitude_fixed = 0,
    .longitude_fixed = 0,
    .altitude = 0,
};

void loop() {
    rockets_client::StaticJsonDoc records = rockets_client::getLatestRecords();
    JsonObject gps = records["GPS"]["data"];

    packet.fix = gps["fix"];

    if (packet.fix) {
        packet.fixquality = gps["fixquality"];
        packet.satellites = gps["satellites"];
        packet.PDOP_10 = (uint8_t)(gps["PDOP"].as<float>() * 10);
        packet.latitude_fixed = gps["latitude_fixed"];
        packet.longitude_fixed = gps["longitude_fixed"];
        packet.altitude = gps["altitude"];
    }

    rf95.send((uint8_t*)&packet, sizeof(packet));
    rf95.waitPacketSent();

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
