#ifndef SERIAL_FORWARDER_H_
#define SERIAL_FORWARDER_H_

#include <Arduino.h>

namespace serialForwarder {

void init();

void tick();

/**
 * Writes the # of bytes in the packet to lenOut and returns a pointer to the
 * packet. If there is no queued packet, lenOut is set to 0. Calling this clears
 * the queued packet.
 */
uint8_t *getQueuedPacket(int *lenOut);

}  // namespace serialForwarder

#endif  // SERIAL_FORWARDER_H_