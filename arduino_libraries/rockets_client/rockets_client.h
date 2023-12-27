#ifndef ROCKETS_CLIENT_H_
#define ROCKETS_CLIENT_H_

#define ARDUINOJSON_USE_LONG_LONG 1

#include <Arduino.h>
#include <ArduinoJson.h>

namespace rockets_client {

struct ServerConfig {
    String host;
    int port;
    String pathPrefix;
};

struct ServerConfigPresets {
    ServerConfig FS_PI;
    ServerConfig ROCKET_PI;
    ServerConfig MECHE;
};

extern ServerConfigPresets serverConfigPresets;

typedef StaticJsonDocument<1024> StaticJsonDoc;
typedef char Buffer[2048];  // give it some extra space

// Returns true if the record was successfully queued. Creates a record with
// appropriate `environmentKey`, `device`, and `ts` fields, and sets the `data`
// field to `recordData`.
bool queueRecord(const StaticJsonDoc& recordData);

// Returns an empty json object ("{}") if there is no message since the last
// call. Must set `pollMessages` to true during init.
StaticJsonDoc getLatestMessage();

// Returns an empty object if unsuccessful or the first poll is still in
// progress. If the object is not empty, then there will be a key for each
// requested device, with a record object or null as the value. This does not
// erase the stored records after retrieving them. Must set `pollRecordsDevices`
// to a non-empty string during init.
StaticJsonDoc getLatestRecords();

void init(ServerConfig serverConfig, String environmentKey, String device,
          bool pollMessages = false, String pollRecordDevices = "");

}  // namespace rockets_client

#endif  // ROCKETS_CLIENT_H_