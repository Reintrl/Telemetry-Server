#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

typedef struct {
    int sensor_id;
    float temperature;
    char status[20];
    char timestamp[20];
} TelemetryData;

void init_data_generator();
TelemetryData generate_telemetry();

#endif