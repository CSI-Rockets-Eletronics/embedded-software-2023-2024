#ifndef ROCKETS_CLIENT_H_
#define ROCKETS_CLIENT_H_

#include <Arduino.h>
#include <ArduinoJson.h>

namespace rockets_client {

typedef StaticJsonDocument<1024> StaticJsonDoc;
typedef char Buffer[2048];  // give it some extra space

// Returns true if the record was successfully queued. Creates a record with
// appropriate `environmentKey`, `path`, and `ts` fields, and sets the `data`
// field to `recordData`.
bool queueRecord(const StaticJsonDoc& recordData);

// Returns an empty json object ("{}") if there is no message since the last
// call.
StaticJsonDoc getLatestMessage();

void init(String host, int port, String pathPrefix, String environmentKey,
          String path);

}  // namespace rockets_client

#endif  // ROCKETS_CLIENT_H_