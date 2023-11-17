#include "rockets_client.h"

#include <WiFi.h>

namespace rockets_client {

// wifi options (should be used by every client)

const char* WIFI_SSID = "Darknet";
const char* WIFI_PASSWORD = "Blueberries";

// server presets

ServerConfigPresets serverConfigPresets = {
    .FS_PI =
        {
            .host = "fs-pi.local",
            .port = 3000,
            .pathPrefix = "",
        },
    .MECHE =
        {
            .host = "csiwiki.me.columbia.edu",
            .port = 3001,
            .pathPrefix = "/rocketsdata2",
        },
};

// task parameters

const int CORE_ID = 0;
const int PRIORITY = 0;
const int STACK_DEPTH = 64 * 1000;  // 64 kB

// constants specific to this client

String HOST;
int PORT;
String PATH_PREFIX;

String ENVIRONMENT_KEY;
String PATH;

// buffers for queued record and latest message

// length 0 if no record is queued
Buffer queuedRecord = "";
// empty if there is no new message since the last retrieval
StaticJsonDoc latestMessage;

// for fetching new messages

int64_t lastMessageTs = 0;

// mutexes for the buffers

SemaphoreHandle_t queuedRecordMutex;
SemaphoreHandle_t latestMessageMutex;

// private functions

// returns true if successful and queued record is non-empty
bool getQueuedRecord(Buffer dest) {
    if (xSemaphoreTake(queuedRecordMutex, portMAX_DELAY)) {
        strncpy(dest, queuedRecord, sizeof(Buffer));
        queuedRecord[0] = '\0';
        xSemaphoreGive(queuedRecordMutex);
        return strlen(dest) > 0;
    } else {
        return false;
    }
}

// returns true if successful
bool setLatestMessage(const Buffer src) {
    StaticJsonDoc doc;
    auto err = deserializeJson(doc, src);

    if (err != DeserializationError::Ok) {
        Serial.print("Message response deserialization failed: ");
        Serial.println(err.f_str());
        return false;
    }

    if (!doc.containsKey("ts")) {
        Serial.println("Message response missing ts");
        return false;
    }

    // no mutex needed; lastMessageTs is only accessed by this task
    lastMessageTs = doc["ts"];

    if (xSemaphoreTake(latestMessageMutex, portMAX_DELAY)) {
        latestMessage = doc;
        xSemaphoreGive(latestMessageMutex);
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

// body is ignored for GET requests
void postReq(String method, WiFiClient& client, String path, const char* body) {
    String fullPath = PATH_PREFIX + path;

    client.setTimeout(3);

    client.print(method);
    client.print(" ");
    client.print(fullPath);
    client.println(" HTTP/1.1");

    client.print("Host: ");
    client.println(HOST);

    client.println("Connection: close");

    if (method == "GET") {
        client.println();
    } else {
        client.println("Content-Type: application/json");

        client.print("Content-Length: ");
        client.println(strlen(body));

        client.println();
        client.print(body);
    }
}

// returns true if successful
bool readRes(WiFiClient& client, Buffer dest) {
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
            if (i < sizeof(Buffer) - 1) {
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
int parseResBody(const Buffer res, Buffer dest) {
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

    strncpy(dest, lineStart, sizeof(Buffer));
    return status;
}

void sendQueuedRecord(WiFiClient& client) {
    Buffer record;
    if (!getQueuedRecord(record)) {
        return;
    }

    if (client.connect(HOST.c_str(), PORT)) {
        Serial.println("Sending queued record");

        postReq("POST", client, "/records", record);

        Buffer res;
        if (readRes(client, res)) {
            Buffer resBody;
            int status = parseResBody(res, resBody);

            if (status == 200) {
                Serial.println("sendQueuedRecord success");
            } else {
                Serial.print("sendQueuedRecord failed, status: ");
                Serial.println(status);
                Serial.println(resBody);
            }
        } else {
            Serial.println("sendQueuedRecord failed, network timeout");
        }
    } else {
        Serial.println("sendQueuedRecord connect failed");
    }

    client.stop();
}

void pollLatestMessage(WiFiClient& client) {
    String path = "/messages/next?environmentKey=" + ENVIRONMENT_KEY +
                  "&path=" + PATH + "&afterTs=" + lastMessageTs;

    if (client.connect(HOST.c_str(), PORT)) {
        Serial.println("Polling latest message");

        postReq("GET", client, path, NULL);

        Buffer res;
        if (readRes(client, res)) {
            Buffer resBody;
            int status = parseResBody(res, resBody);

            if (status == 200) {
                Serial.println("pollLatestMessage success");

                if (strcmp(resBody, "NONE") != 0) {
                    setLatestMessage(resBody);
                }
            } else {
                Serial.print("pollLatestMessage failed, status: ");
                Serial.println(status);
                Serial.println(resBody);
            }
        } else {
            Serial.println("pollLatestMessage failed, network timeout");
        }
    } else {
        Serial.println("pollLatestMessage connect failed");
    }

    client.stop();
}

void runTask(void* pvParameters) {
    WiFiClient client;

    while (true) {
        printHeartbeat();

        sendQueuedRecord(client);
        delay(5);
        pollLatestMessage(client);
        delay(5);
    }
}

void initTask() {
    queuedRecordMutex = xSemaphoreCreateMutex();
    latestMessageMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(runTask, "network", STACK_DEPTH, NULL, PRIORITY,
                            NULL, CORE_ID);
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

// implementation of the interface

bool queueRecord(const StaticJsonDoc& recordData) {
    StaticJsonDoc record;

    record["environmentKey"] = ENVIRONMENT_KEY;
    record["path"] = PATH;
    record["ts"] = esp_timer_get_time();

    JsonObject data = record.createNestedObject("data");
    data.set(recordData.as<JsonObjectConst>());

    if (xSemaphoreTake(queuedRecordMutex, portMAX_DELAY)) {
        serializeJson(record, queuedRecord, sizeof(Buffer));
        xSemaphoreGive(queuedRecordMutex);
        return true;
    } else {
        return false;
    }
}

// Returns an empty object if unsuccessful or there is no new message.
StaticJsonDoc getLatestMessage() {
    StaticJsonDoc doc;

    if (xSemaphoreTake(latestMessageMutex, portMAX_DELAY)) {
        doc = latestMessage;
        latestMessage = StaticJsonDoc();
        xSemaphoreGive(latestMessageMutex);
        return doc;
    } else {
        // return empty json object
        return doc;
    }
}

void init(ServerConfig serverConfig, String environmentKey, String path) {
    HOST = serverConfig.host;
    PORT = serverConfig.port;
    PATH_PREFIX = serverConfig.pathPrefix;

    ENVIRONMENT_KEY = environmentKey;
    PATH = path;

    initWifi();
    initTask();
}

}  // namespace rockets_client