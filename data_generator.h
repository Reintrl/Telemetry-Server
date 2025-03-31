#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H
#include <stdint.h>

typedef struct {
    int sensor_id;
    float temperature;
    char status[20];
    uint32_t timestamp;
} TelemetryData;

void init_data_generator();
TelemetryData generate_telemetry();

#endif