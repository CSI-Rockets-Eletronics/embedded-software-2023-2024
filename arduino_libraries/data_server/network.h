#ifndef NETWORK_H_
#define NETWORK_H_

namespace network {

bool setStateReqBody(const char* bodyStr);
bool getCommandResBody(char* destBuf, int bufSize);
void init();

}  // namespace network

#endif  // NETWORK_H_