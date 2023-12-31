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

    rockets_client::init(rockets_client::serverConfigPresets.ROCKET_PI, "0",
                         "RadioGround");
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

    gps["fix"] = packet->fix;
    if (packet->fix) {
        gps["fixquality"] = packet->fixquality;
        gps["satellites"] = packet->satellites;
        gps["PDOP"] = packet->PDOP_10 / 10.0;
        gps["latitude_fixed"] = packet->latitude_fixed;
        gps["longitude_fixed"] = packet->longitude_fixed;
        gps["altitude"] = packet->altitude;
    }

    rockets_client::queueRecord(recordData);

    // print packet

    Serial.print("fix: ");
    Serial.print(packet->fix);

    if (packet->fix) {
        Serial.print("\tfixquality: ");
        Serial.print(packet->fixquality);

        Serial.print("\tsatellites: ");
        Serial.print(packet->satellites);

        Serial.print("\tPDOP: ");
        Serial.print(packet->PDOP_10 / 10.0);

        Serial.print("\tlatitude_fixed: ");
        Serial.print(packet->latitude_fixed);

        Serial.print("\tlongitude_fixed: ");
        Serial.print(packet->longitude_fixed);

        Serial.print("\taltitude: ");
        Serial.print(packet->altitude);
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
