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

#define SCLK 12
#define MISO 13
#define MOSI 11
#define SS 10
#define INT 2

// SPIClass spi(HSPI);

RH_RF95 rf95(SS, INT);

// Singleton instance of the radio driver
// RH_RF95 rf95(10, 18);
// Setting the pins for the ESP32
// RH_RF95 rf95(15, 16); // For RF95 on PJRC breakout board with Teensy 3.1
// RH_RF95 rf95(4, 2); // For MoteinoMEGA
// https://lowpowerlab.com/shop/moteinomega RH_RF95 rf95(8, 7); // Adafruit
// Feather 32u4

void setup() {
    Serial.begin(115200);
    while (!Serial)
        ;
	
    // spi.begin(SCLK, MISO, MOSI, SS);  // Set the SPI pins
    // rf95.setSPI(spi); // Set the SPI instance for the RH_RF95 object
    if (!rf95.init()) Serial.println("init failed");
    // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for
    // low power module) No encryption
    if (!rf95.setFrequency(434.0)) Serial.println("setFrequency failed");

    // If you are using a high power RF95 eg RFM95HW, you *must* set a Tx power
    // with the ishighpowermodule flag set like this:
    // rf95.setTxPower(14, true);

    // The encryption key has to be the same as the one in the client
    // uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    // 				  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
    // 				};
    // rf95.setEncryptionKey(key);

    // #if 0
    // 	// For compat with RFM95 Struct_send
    // 	rf95.setModemConfig(RH_RF95::GFSK_Rb250Fd250);
    // 	rf95.setPreambleLength(3);
    // 	uint8_t syncwords[] = { 0x2d, 0x64 };
    // 	rf95.setSyncWords(syncwords, sizeof(syncwords));
    // 	rf95.setEncryptionKey((uint8_t*)"thisIsEncryptKey");
    // #endif
}

void loop() {
    if (rf95.available()) {
        // Should be a message for us now
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        if (rf95.recv(buf, &len)) {
            //      RH_RF95::printBuffer("request: ", buf, len);
            Serial.print("got request: ");
            Serial.println((char*)buf);
            //      Serial.print("RSSI: ");
            //      Serial.println(rf95.lastRssi(), DEC);

            // Send a reply
            uint8_t data[] = "And hello back to you";
            rf95.send(data, sizeof(data));
            rf95.waitPacketSent();
            Serial.println("Sent a reply");
        } else {
            Serial.println("recv failed");
        }
    }
}
