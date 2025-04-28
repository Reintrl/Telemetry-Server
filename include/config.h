#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

typedef struct {
    // Server settings
    int port;
    int max_clients;
    int update_interval_ms;

    // Data generator settings
    int min_sensor_id;
    int max_sensor_id;
    float min_temperature;
    float max_temperature;
    char status_options[3][20]; // Possible status values

    // Logging settings
    char log_file[256];
    int log_to_console;
    int log_level; // 0-error, 1-warn, 2-info, 3-debug

    // Serialization settings
    char serialization_format[10]; // "json", "xml", etc.
} ServerConfig;

void load_config(const char* filename);
const ServerConfig* get_config();
void print_config();

#endif