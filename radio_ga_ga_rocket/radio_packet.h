struct RadioPacket {
    // 4 bytes
    bool fix;
    uint8_t fixquality;
    uint8_t satellites;
    uint8_t PDOP;  // cast float PDOP to int
    // 4 bytes
    int32_t latitude_fixed;
    // 4 bytes
    int32_t longitude_fixed;
    // 4 bytes
    float altitude;
};
