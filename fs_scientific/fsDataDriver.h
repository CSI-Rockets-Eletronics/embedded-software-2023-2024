#ifndef FS_DATA_DRIVER_H_
#define FS_DATA_DRIVER_H_

namespace fsDataDriver {

const int SCIENTIFIC_DATA_PACKET_SIZE = 32;

struct ScientificDataPacket {
    int64_t time;
    int32_t oxTankPressure;
    int32_t ccPressure;
    int32_t oxidizerTankTransducerValue;
    int32_t combustionChamberTransducerValue;
    byte timeSinceLastCalibration;
    byte timeSinceLastStartup;

    void dumpPacket(byte inBuffer[]);
};

inline void ScientificDataPacket::dumpPacket(byte inBuffer[]) {
    static const byte scientificPacketIdentifier = 202;
    static const byte finalPacketPadding[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

    memcpy(inBuffer, &scientificPacketIdentifier, 1);
    memcpy(inBuffer + 1, &time, 8);
    memcpy(inBuffer + 9, &oxTankPressure, 4);
    memcpy(inBuffer + 13, &ccPressure, 4);
    memcpy(inBuffer + 17, &oxidizerTankTransducerValue, 4);
    memcpy(inBuffer + 21, &combustionChamberTransducerValue, 4);
    memcpy(inBuffer + 25, &timeSinceLastCalibration, 1);
    memcpy(inBuffer + 26, &timeSinceLastStartup, 1);
    // Padding to get to 32 bytes
    memcpy(inBuffer + 27, finalPacketPadding, 5);
}

}  // namespace fsDataDriver

#endif  // FS_DATA_DRIVER_H_