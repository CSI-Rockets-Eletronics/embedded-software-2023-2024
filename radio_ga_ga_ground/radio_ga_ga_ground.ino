// rf95_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging server
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_client
// Demonstrates the use of AES encryption, setting the frequency and modem
// configuration.
// Tested on Moteino with RFM95 http://lowpowerlab.com/moteino/
// Tested on miniWireless with RFM95 www.anarduino.com/miniwireless
// Tested on Teensy 3.1 with RF95 on PJRC breakout board

#include <RH_RF95.h>
#include <SPI.h>
#include <rockets_client.h>

#include "radio_packet.h"

const bool UPLOAD_TO_SERVER = true;

// Singleton instance of the radio driver
RH_RF95 rf95;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    // spi.begin(SCLK, MISO, MOSI, SS);  // Set the SPI pins
    // rf95.setSPI(spi); // Set the SPI instance for the RH_RF95 object
    if (!rf95.init()) Serial.println("init failed");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for
    // low power module) No encryption
    if (!rf95.setFrequency(434.0)) Serial.println("setFrequency failed");

    // If you are using a high power RF95 eg RFM95HW, you *must* set a Tx power
    // with the ishighpowermodule flag set like this:
    rf95.setTxPower(23, false);

    // The encryption key has to be the same as the one in the client
    // uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    // 				  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    // 				};
    // rf95.setEncryptionKey(key);

    if (UPLOAD_TO_SERVER) {
        rockets_client::init(
            rockets_client::serverConfigPresets.ALEX_HOME_DESKTOP, "0",
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

    // if (rf95.available()) {
    //     // Should be a message for us now
    //     uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    //     uint8_t len = sizeof(buf);
    //     if (rf95.recv(buf, &len)) {
    //         //      RH_RF95::printBuffer("request: ", buf, len);
    //         Serial.print("got request: ");
    //         Serial.println((char*)buf);
    //         //      Serial.print("RSSI: ");
    //         //      Serial.println(rf95.lastRssi(), DEC);

    //         // Send a reply
    //         uint8_t data[] = "And hello back to you";
    //         rf95.send(data, sizeof(data));
    //         rf95.waitPacketSent();
    //         Serial.println("Sent a reply");
    //     } else {
    //         Serial.println("recv failed");
    //     }
    // }
}
