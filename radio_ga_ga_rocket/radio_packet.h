struct RadioPacket {
    // ========== 4 bytes ==========

    // last byte of microsecond timestamp of gps micro-controller, to detect if
    // we're receiving fresh gps data
    uint8_t gps_ts_tail;
    bool gps_fix;
    uint8_t gps_fixquality;
    uint8_t gps_satellites;

    // ========== 4 bytes ==========

    int32_t gps_latitude_fixed;

    // ========== 4 bytes ==========

    int32_t gps_longitude_fixed;

    // ========== 4 bytes ==========

    float gps_altitude;
};
