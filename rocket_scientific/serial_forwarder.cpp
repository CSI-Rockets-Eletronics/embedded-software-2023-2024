#include "serial_forwarder.h"

namespace serialForwarder {

const int RX_PIN = 47;
const int TX_PIN = 48;

// worst case, the delimiter occurs in the data, in which case we drop a packet
uint8_t PACKET_DELIMITER[] = {0b10101010, 0b01010101};
const int BUFFER_SIZE = 128;

uint8_t buffer[BUFFER_SIZE];
int bufferIndex = 0;  // equals the current size of the buffer

// end of packet is marked by PACKET_DELIMITER
uint8_t queuedPacket[BUFFER_SIZE];
bool hasQueuedPacket = false;

void init() { Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); }

void tick() {
    while (Serial1.available()) {
        // if the buffer is full, just empty it lol
        if (bufferIndex == BUFFER_SIZE) {
            bufferIndex = 0;
        }

        uint8_t c = Serial1.read();

        buffer[bufferIndex] = c;
        bool hitDelimeter = bufferIndex > 0 &&
                            buffer[bufferIndex - 1] == PACKET_DELIMITER[0] &&
                            buffer[bufferIndex] == PACKET_DELIMITER[1];

        if (hitDelimeter) {
            hasQueuedPacket = true;
            // bufferIndex still points to the last byte, so bufferIndex + 1 is
            // the packet size
            memcpy(queuedPacket, buffer, bufferIndex + 1);
            bufferIndex = 0;
        } else {
            bufferIndex++;
        }
    }
}

uint8_t *getQueuedPacket(int *lenOut) {
    if (!hasQueuedPacket) {
        *lenOut = 0;
        return queuedPacket;
    }

    *lenOut = 0;  // in case we don't find the delimiter

    // find the delimiter
    for (int i = 1; i < BUFFER_SIZE; i++) {
        bool hitDelimeter = buffer[i - 1] == PACKET_DELIMITER[0] &&
                            buffer[i] == PACKET_DELIMITER[1];

        if (hitDelimeter) {
            // include delimeter bytes in the packet
            *lenOut = i + 1;
            break;
        }
    }

    hasQueuedPacket = false;
    return queuedPacket;
}

}  // namespace serialForwarder