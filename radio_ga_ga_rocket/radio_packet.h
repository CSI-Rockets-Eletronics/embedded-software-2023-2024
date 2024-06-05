struct RadioPacket {
    // .......... GPS ..........

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

    // .......... Trajectory ..........

    // ========== 4 bytes ==========
    float trajectory_z;
    // ========== 4 bytes ==========
    float trajectory_vz;
    // ========== 4 bytes ==========
    float trajectory_az;

    // .......... RocketScientific ..........

    // ========== 2 bytes ==========
    int16_t rocket_scientific_transducer1;
    // ========== 2 bytes ==========
    int16_t rocket_scientific_transducer3;
};
