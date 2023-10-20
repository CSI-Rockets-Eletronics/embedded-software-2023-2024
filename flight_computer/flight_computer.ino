// #include <DHT.h>

// #define DHTPIN 2

// DHT dht(DHTPIN, DHT11);

// void setup() {
//     dht.begin();

//     Serial.begin(115200);
//     delay(1000);
// }

// void loop() {
//     float temp = dht.readTemperature(true);
//     float hum = dht.readHumidity();

//     Serial.print("Temperature: ");
//     Serial.print(temp);
//     Serial.print(" F");
//     Serial.print("\t");

//     Serial.print("Humidity: ");
//     Serial.print(hum);
//     Serial.print(" %");
//     Serial.println();
// }

#include <MPU9255.h>

MPU9255 mpu;

void setup() {
    Serial.begin(115200);  // initialize Serial port

    if (mpu.init(21, 20)) {
        Serial.println("initialization failed");
    } else {
        Serial.println("initialization successful!");
    }
}

void loop() {
    mpu.read_acc();   // get data from the accelerometer
    mpu.read_gyro();  // get data from the gyroscope
    mpu.read_mag();   // get data from the magnetometer

    // print all data in serial monitor
    Serial.print("AX: ");
    Serial.print(mpu.ax);
    Serial.print(" AY: ");
    Serial.print(mpu.ay);
    Serial.print(" AZ: ");
    Serial.print(mpu.az);
    Serial.print("    GX: ");
    Serial.print(mpu.gx);
    Serial.print(" GY: ");
    Serial.print(mpu.gy);
    Serial.print(" GZ: ");
    Serial.print(mpu.gz);
    Serial.print("    MX: ");
    Serial.print(mpu.mx);
    Serial.print(" MY: ");
    Serial.print(mpu.my);
    Serial.print(" MZ: ");
    Serial.println(mpu.mz);
    delay(100);
}
