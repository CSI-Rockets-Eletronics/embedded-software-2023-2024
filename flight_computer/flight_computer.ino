#include <MPU9255.h>

#include "frequency_logger.h"

// remember to connect TX to RX and RX to TX
const int RX_PIN = 47;
const int TX_PIN = 48;

MPU9255 mpu;

// for logging
FrequencyLogger frequencyLogger = FrequencyLogger("flight computer", 1000);

// https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
inline uint64_t htonll(uint64_t x) {
    return ((1 == htonl(1))
                ? (x)
                : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32));
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    // for Raspberry Pi
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    if (mpu.init(21, 20)) {
        Serial.println("initialization failed");
    } else {
        Serial.println("initialization successful!");
    }

    // save power
    mpu.disable(magnetometer);
    mpu.disable(thermometer);

    // set bandwidth
    mpu.set_acc_bandwidth(acc_460Hz);
    mpu.set_gyro_bandwidth(gyro_250Hz);

    // set scale
    mpu.set_acc_scale(scale_16g);
    mpu.set_gyro_scale(scale_2000dps);
}

void loop() {
    frequencyLogger.tick();

    int64_t ts_host = esp_timer_get_time();

    mpu.read_acc();   // get data from the accelerometer
    mpu.read_gyro();  // get data from the gyroscope

    // deal with endianness
    int64_t ts = htonll(ts_host);
    int16_t ax = htons(mpu.ax);
    int16_t ay = htons(mpu.ay);
    int16_t az = htons(mpu.az);
    int16_t gx = htons(mpu.gx);
    int16_t gy = htons(mpu.gy);
    int16_t gz = htons(mpu.gz);

    Serial2.write((uint8_t *)&ts, sizeof(ts));  // 8 bytes
    Serial2.write((uint8_t *)&ax, sizeof(ax));  // 2 bytes
    Serial2.write((uint8_t *)&ay, sizeof(ay));  // 2 bytes
    Serial2.write((uint8_t *)&az, sizeof(az));  // 2 bytes
    Serial2.write((uint8_t *)&gx, sizeof(gx));  // 2 bytes
    Serial2.write((uint8_t *)&gy, sizeof(gy));  // 2 bytes
    Serial2.write((uint8_t *)&gz, sizeof(gz));  // 2 bytes

    // worst case, the delimiter occurs in the data,
    // in which case we drop a packet
    uint8_t packet_delimiter[] = {0b10101010, 0b01010101};
    Serial2.write(packet_delimiter, sizeof(packet_delimiter));
}
