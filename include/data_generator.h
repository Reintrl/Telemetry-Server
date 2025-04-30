#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

typedef struct {
    char sensor_type[20];
    int sensor_id;
    double value;
    char value_str[32];
    char unit[10];
    char timestamp[20];
} TelemetryData;

void init_data_generator();
TelemetryData generate_telemetry();
TelemetryData generate_telemetry_by_type(const char* sensor_type);
TelemetryData generate_telemetry_by_id(int sensor_id);

#endif