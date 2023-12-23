// Test code for Ultimate GPS Using Hardware Serial (e.g. GPS Flora or
// FeatherWing)
//
// This code shows how to listen to the GPS module via polling. Best used with
// Feathers or Flora where you have hardware Serial and no interrupt
//
// Tested and works great with the Adafruit GPS FeatherWing
// ------> https://www.adafruit.com/products/3133
// or Flora GPS
// ------> https://www.adafruit.com/products/1059
// but also works with the shield, breakout
// ------> https://www.adafruit.com/products/1272
// ------> https://www.adafruit.com/products/746
//
// Pick one up today at the Adafruit electronics shop
// and help support open source hardware & software! -ada

#include <Adafruit_GPS.h>
#include <rockets_client.h>

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false
#define GPS_PRINT_NMEA false

#define RXPIN 47
#define TXPIN 48

uint32_t timer = millis();

void setup() {
    // while (!Serial);  // uncomment to have the sketch wait until Serial is
    // ready

    // connect at 115200 so we can read the GPS fast enough and echo without
    // dropping chars also spit it out
    Serial.begin(115200);
    Serial.println("Adafruit GPS library basic parsing test!");

    Serial1.setPins(RXPIN, TXPIN);
    // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
    GPS.begin(9600);
    // uncomment this line to turn on RMC (recommended minimum) and GGA (fix
    // data) including altitude
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // uncomment this line to turn on only the "minimum recommended" data
    // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    // For parsing data, we don't suggest using anything but either RMC only or
    // RMC+GGA since the parser doesn't care about other sentences at this time
    // Set the update rate
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_10HZ);  // 1 Hz update rate
    // For the parsing code to work nicely and have time to sort thru the data,
    // and print it out we don't suggest using anything higher than 1 Hz

    // Request updates on antenna status, comment out to keep quiet
    GPS.sendCommand(PGCMD_ANTENNA);

    delay(1000);

    // Ask for firmware version
    GPSSerial.println(PMTK_Q_RELEASE);

    rockets_client::init(rockets_client::serverConfigPresets.ROCKET_PI, "0",
                         "GPS");
}

void loop() {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();

    // if you want to debug, this is a good time to do it!
    if (GPSECHO) {
        if (c) Serial.print(c);
    }

    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
        // a tricky thing here is if we print the NMEA sentence, or data
        // we end up not listening and catching other sentences!
        // so be very wary if using OUTPUT_ALLDATA and trying to print out data
        if (GPS_PRINT_NMEA) {
            Serial.print(GPS.lastNMEA());
        }

        // this also sets the newNMEAreceived() flag to false
        if (!GPS.parse(GPS.lastNMEA())) {
            // we can fail to parse a sentence in which case we should just wait
            // for another
            Serial.println("Failed to parse GPS data");
            return;
        }

        // send data to server
        rockets_client::StaticJsonDoc recordData;

        recordData["fix"] = GPS.fix;
        if (GPS.fix) {
            recordData["fixquality"] = GPS.fixquality;
            recordData["latitude_fixed"] = GPS.latitude_fixed;
            recordData["longitude_fixed"] = GPS.longitude_fixed;
            recordData["speed"] = GPS.speed;
            recordData["angle"] = GPS.angle;
            recordData["altitude"] = GPS.altitude;
            recordData["satellites"] = GPS.satellites;
            recordData["antenna"] = GPS.antenna;
            recordData["PDOP"] = GPS.PDOP;
        }

        rockets_client::queueRecord(recordData);
    }

    // approximately every 2 seconds or so, print out the current stats
    if (millis() - timer > 2000) {
        timer = millis();  // reset the timer
        Serial.print("\nTime: ");
        if (GPS.hour < 10) {
            Serial.print('0');
        }
        Serial.print(GPS.hour, DEC);
        Serial.print(':');
        if (GPS.minute < 10) {
            Serial.print('0');
        }
        Serial.print(GPS.minute, DEC);
        Serial.print(':');
        if (GPS.seconds < 10) {
            Serial.print('0');
        }
        Serial.print(GPS.seconds, DEC);
        Serial.print('.');
        if (GPS.milliseconds < 10) {
            Serial.print("00");
        } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
            Serial.print("0");
        }
        Serial.println(GPS.milliseconds);
        Serial.print("Date: ");
        Serial.print(GPS.day, DEC);
        Serial.print('/');
        Serial.print(GPS.month, DEC);
        Serial.print("/20");
        Serial.println(GPS.year, DEC);
        Serial.print("Fix: ");
        Serial.print((int)GPS.fix);
        Serial.print(" quality: ");
        Serial.println((int)GPS.fixquality);
        if (GPS.fix) {
            Serial.print("Location: ");
            Serial.print(GPS.latitude, 4);
            Serial.print(GPS.lat);
            Serial.print(", ");
            Serial.print(GPS.longitude, 4);
            Serial.println(GPS.lon);
            Serial.print("Speed (knots): ");
            Serial.println(GPS.speed);
            Serial.print("Angle: ");
            Serial.println(GPS.angle);
            Serial.print("Altitude: ");
            Serial.println(GPS.altitude);
            Serial.print("Satellites: ");
            Serial.println((int)GPS.satellites);
            Serial.print("Antenna status: ");
            Serial.println((int)GPS.antenna);
        }
    }
}