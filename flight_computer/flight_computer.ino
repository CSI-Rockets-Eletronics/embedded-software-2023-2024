#include <Arduino.h>
#include <DHT.h>
#include <MPU9255.h>

#include "frequency_logger.h"

// remember to connect TX to RX and RX to TX
const int RX_PIN = 47;
const int TX_PIN = 48;

const int DHT_PIN = 2;

// worst case, the delimiter occurs in the data, in which case we drop a packet
uint8_t PACKET_DELIMITER[] = {0b10101010, 0b01010101};

MPU9255 mpu;
DHT dht(DHT_PIN, DHT11);

SemaphoreHandle_t piSerialMutex;

// for logging
FrequencyLogger mpuFreqLogger = FrequencyLogger("MPU", 1000);
FrequencyLogger dhtFreqLogger = FrequencyLogger("DHT", 1000);

// https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
inline uint64_t htonll(uint64_t x) {
    return ((1 == htonl(1))
                ? (x)
                : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32));
}

void mpuLoop() {
    mpuFreqLogger.tick();

    int64_t ts_host = esp_timer_get_time();

    mpu.read_acc();   // get data from the accelerometer
    mpu.read_gyro();  // get data from the gyroscope

    // deal with endianness
    uint64_t ts = htonll(ts_host);
    uint16_t ax = htons(mpu.ax);
    uint16_t ay = htons(mpu.ay);
    uint16_t az = htons(mpu.az);
    uint16_t gx = htons(mpu.gx);
    uint16_t gy = htons(mpu.gy);
    uint16_t gz = htons(mpu.gz);

    Serial2.write((uint8_t *)&ts, sizeof(ts));  // 8 bytes
    Serial2.write((uint8_t *)&ax, sizeof(ax));  // 2 bytes
    Serial2.write((uint8_t *)&ay, sizeof(ay));  // 2 bytes
    Serial2.write((uint8_t *)&az, sizeof(az));  // 2 bytes
    Serial2.write((uint8_t *)&gx, sizeof(gx));  // 2 bytes
    Serial2.write((uint8_t *)&gy, sizeof(gy));  // 2 bytes
    Serial2.write((uint8_t *)&gz, sizeof(gz));  // 2 bytes

    Serial2.write(PACKET_DELIMITER, sizeof(PACKET_DELIMITER));
}

void dhtLoop() {
    dhtFreqLogger.tick();

    int64_t ts_host = esp_timer_get_time();

    // force read once for both temperature and humidity
    bool success = dht.read(true);
    if (!success) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // these will use the values from the last read()
    float temp_host = dht.readTemperature();
    float hum_host = dht.readHumidity();

    // deal with endianness
    uint32_t temp = htonl(*((uint32_t *)&temp_host));
    uint32_t hum = htonl(*((uint32_t *)&hum_host));

    Serial2.write((uint8_t *)&ts_host, sizeof(ts_host));  // 8 bytes
    Serial2.write((uint8_t *)&temp, sizeof(temp));        // 4 bytes
    Serial2.write((uint8_t *)&hum, sizeof(hum));          // 4 bytes

    Serial2.write(PACKET_DELIMITER, sizeof(PACKET_DELIMITER));
}

void runDhtTask(void *pvParameters) {
    while (true) {
        dhtLoop();
    }
}

void setup() {
    // ========== Serial setup ==========

    Serial.begin(115200);
    while (!Serial && millis() < 500)
        ;  // wait up to 500ms for serial to connect; needed for native USB

    // for Raspberry Pi
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // ========== MPU setup ==========

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

    // ========== DHT setup ==========

    dht.begin();

    // ========== Threading setup ==========

    piSerialMutex = xSemaphoreCreateMutex();

    // task parameters
    const int CORE_ID = 0;
    const int PRIORITY = 0;
    const int STACK_DEPTH = 64 * 1000;  // 64 kB

    xTaskCreatePinnedToCore(runDhtTask, "DHT", STACK_DEPTH, NULL, PRIORITY,
                            NULL, CORE_ID);
}

void loop() {
    mpuLoop();
    // dhtLoop is handled by in the other thread
}
