#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// uncomment below to use online server
#define ONLINE_SERVER

namespace constants {

// server options
#ifdef ONLINE_SERVER
inline const String HOST = "csiwiki.me.columbia.edu";
inline const int PORT = 80;
inline const String PATH_PREFIX = "/rocketsdata";
inline const char* STATION_ID = "cl9vt57vf0000qw4nmwr6glcm";
#else
inline const String HOST = "192.168.137.1";
inline const int PORT = 3000;
inline const String PATH_PREFIX = "";
inline const char* STATION_ID = "0";
#endif

// API options
inline const char* OP_STATE_MESSAGE_TARGET = "FiringStation";
inline const char* STATION_STATE_RECORD_SOURCE = "FiringStation";

}  // namespace constants

#endif  // CONSTANTS_H_