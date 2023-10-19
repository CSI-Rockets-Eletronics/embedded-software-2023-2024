#include "network.h"

#include <Arduino.h>
#include <WiFi.h>

#include "constants.h"

namespace network {

// wifi options
const char* WIFI_SSID = "Darknet";
const char* WIFI_PASSWORD = "Blueberries";

// task parameters
const int CORE_ID = 0;
const int PRIORITY = 0;
const int STACK_DEPTH = 64 * 1000;  // 64 kB

const int BODY_LEN = 2048;

char stateReqBody[BODY_LEN] = "";
char commandResBody[BODY_LEN] = "";

SemaphoreHandle_t stateBodyMutex;
SemaphoreHandle_t commandBodyMutex;

bool getStateReqBody(char* dest, int destSize) {
    if (xSemaphoreTake(stateBodyMutex, portMAX_DELAY)) {
        strncpy(dest, stateReqBody, destSize);
        stateReqBody[0] = '\0';
        xSemaphoreGive(stateBodyMutex);
        return strlen(dest) > 0;
    } else {
        return false;
    }
}

bool setStateReqBody(const char* body) {
    if (xSemaphoreTake(stateBodyMutex, portMAX_DELAY)) {
        strncpy(stateReqBody, body, BODY_LEN);
        xSemaphoreGive(stateBodyMutex);
        return true;
    } else {
        return false;
    }
}

bool getCommandResBody(char* dest, int destSize) {
    if (xSemaphoreTake(commandBodyMutex, portMAX_DELAY)) {
        strncpy(dest, commandResBody, destSize);
        commandResBody[0] = '\0';
        xSemaphoreGive(commandBodyMutex);
        return strlen(dest) > 0;
    } else {
        return false;
    }
}

bool setCommandResBody(const char* body) {
    if (xSemaphoreTake(commandBodyMutex, portMAX_DELAY)) {
        strncpy(commandResBody, body, BODY_LEN);
        xSemaphoreGive(commandBodyMutex);
        return true;
    } else {
        return false;
    }
}

void printHeartbeat() {
    Serial.print("Free heap memory (bytes): ");
    Serial.println(esp_get_free_heap_size());

    Serial.print("Network heartbeat: ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");
    } else {
        Serial.println("WiFi not connected");
    }
}

void postReq(WiFiClient& client, String path, const char* body) {
    client.setTimeout(3);

    client.print("POST ");
    client.print(path);
    client.println(" HTTP/1.1");

    client.print("Host: ");
    client.println(constants::HOST);

    client.println("Connection: close");
    client.println("Content-Type: application/json");

    client.print("Content-Length: ");
    client.println(strlen(body));

    client.println();
    client.print(body);
}

// returns true if successful
bool readRes(WiFiClient& client, char* dest, int destSize) {
    int TIMEOUT = 3000;

    int i = 0;
    int startTime = millis();

    while (client.connected()) {
        if (millis() - startTime > TIMEOUT) {
            return false;
        }

        while (client.available()) {
            if (millis() - startTime > TIMEOUT) {
                return false;
            }

            char c = client.read();
            if (i < destSize - 1) {
                dest[i] = c;
                i++;
            }
        }

        yield();
    }

    dest[i] = '\0';
    return true;
}

// returns HTTP status, or 0 if error
int parseResBody(const char* res, char* dest, int destSize) {
    int status;

    bool readStatus = sscanf(res, "HTTP/1.1 %d", &status);
    if (!readStatus) {
        return 0;
    }

    // read until empty line (end of headers)
    const char* lineStart = res;
    while (true) {
        const char* lineEnd = strstr(lineStart, "\r\n");
        if (lineEnd == NULL) {
            return 0;
        }
        if (lineEnd == lineStart) {
            lineStart += 2;
            break;
        }
        lineStart = lineEnd + 2;
    }

    strncpy(dest, lineStart, destSize);
    return status;
}

void sendState(WiFiClient& client) {
    char reqBody[BODY_LEN];
    if (!getStateReqBody(reqBody, BODY_LEN)) {
        return;
    }

    if (strlen(reqBody) == 0) {
        return;
    }

    String path = constants::PATH_PREFIX + "/record";

    if (client.connect(constants::HOST.c_str(), constants::PORT)) {
        Serial.println("Sending state");

        postReq(client, path, reqBody);

        char res[BODY_LEN];
        if (readRes(client, res, BODY_LEN)) {
            char resBody[BODY_LEN];
            int status = parseResBody(res, resBody, BODY_LEN);

            if (status == 200) {
                Serial.print("sendState success: ");
                Serial.println(resBody);
            } else {
                Serial.print("sendState failed, status: ");
                Serial.println(status);
                Serial.println(resBody);
            }
        } else {
            Serial.println("sendState failed, network timeout");
        }
    } else {
        Serial.println("sendState connect failed");
    }

    client.stop();
}

void pollCommand(WiFiClient& client) {
    char reqBody[BODY_LEN];
    snprintf(reqBody, BODY_LEN, "{\"stationId\":\"%s\",\"target\":\"%s\"}",
             constants::STATION_ID, constants::OP_STATE_MESSAGE_TARGET);

    String path = constants::PATH_PREFIX + "/message/next";

    if (client.connect(constants::HOST.c_str(), constants::PORT)) {
        Serial.println("Polling command");

        postReq(client, path, reqBody);

        char res[BODY_LEN];
        if (readRes(client, res, BODY_LEN)) {
            char resBody[BODY_LEN];
            int status = parseResBody(res, resBody, BODY_LEN);

            if (status == 200) {
                Serial.println("pollCommand success");
                setCommandResBody(resBody);
            } else {
                Serial.print("pollCommand failed, status: ");
                Serial.println(status);
                Serial.println(resBody);
            }
        } else {
            Serial.println("pollCommand failed, network timeout");
        }
    } else {
        Serial.println("pollCommand connect failed");
    }

    client.stop();
}

void runTask(void* pvParameters) {
    WiFiClient client;

    while (true) {
        printHeartbeat();

        sendState(client);
        delay(5);
        pollCommand(client);
        delay(5);
    }
}

void initWifi() {
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // block until connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void initTask() {
    stateBodyMutex = xSemaphoreCreateMutex();
    commandBodyMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(runTask, "network", STACK_DEPTH, NULL, PRIORITY,
                            NULL, CORE_ID);
}

void init() {
    initWifi();
    initTask();
}

}  // namespace network