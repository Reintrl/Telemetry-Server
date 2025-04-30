#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

typedef struct {
    char sensor_type[20];
    int min_id;
    int max_id;
    double min_value;
    double max_value;
    char unit[10];
} SensorConfig;

typedef struct {
    // Server settings
    int port;
    int max_clients;
    int max_sensors;  // Добавлено
    int update_interval_ms;

    // Logging settings
    char log_file[256];
    int log_to_console;
    int log_level;

    // Serialization settings
    char serialization_format[10];

    // Sensor configurations
    SensorConfig* sensor_configs;
    int sensor_config_count;
} ServerConfig;

void load_config(const char* filename);
void load_sensor_configs(ServerConfig* config, const char** config_files, int count);
const ServerConfig* get_config();
void print_config();
const SensorConfig* get_sensor_config(const char* sensor_type);

#endif