typedef struct __attribute__((__packed__)) {
    unsigned int temperature : 16;
    unsigned int pressure : 16;
    unsigned int humidity : 16;
    unsigned int gas_sensor1 : 16;
    unsigned int gas_sensor2 : 16;
    unsigned int gas_sensor3 : 16;
    unsigned int gyro0 : 12;
    unsigned int gyro1 : 12;
    unsigned int gyro2 : 12;
    unsigned int gyro3 : 12;
    unsigned int gyro4 : 12;
    unsigned int gyro5 : 12;
    unsigned int gyro6 : 12;
    unsigned int gyro7 : 12;
    unsigned int gyro8 : 12;
    unsigned int photoresistor : 16;
    unsigned int realtimeclock : 32;
    unsigned int mvps_voltage : 16;
    unsigned int mvps_current : 16;
    unsigned int radiation : 16;
    unsigned int cosmic_rays : 16;
    unsigned int pad1 : 12;
    unsigned int pad2 : 32;
    unsigned int pad3 : 32;
    unsigned int pad4 : 32;
    unsigned int pad5 : 32;
} exptelemetry_t;



