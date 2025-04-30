#include "../include/config.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <strings.h> // для strcasecmp

static ServerConfig config = {0};

static void read_json_string(struct json_object* obj, const char* key,
                           char* dest, size_t dest_size, const char* default_val) {
    struct json_object* tmp;
    if (json_object_object_get_ex(obj, key, &tmp)) {
        const char* str = json_object_get_string(tmp);
        strncpy(dest, str, dest_size - 1);
        dest[dest_size - 1] = '\0';
    } else if (default_val) {
        strncpy(dest, default_val, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

void load_sensor_config(const char* filename, SensorConfig* config) {
    struct json_object* json = json_object_from_file(filename);
    if (!json) {
        fprintf(stderr, "Warning: Could not load sensor config file '%s'\n", filename);
        return;
    }

    read_json_string(json, "sensor_type", config->sensor_type, sizeof(config->sensor_type), "");
    config->min_id = json_object_get_int(json_object_object_get(json, "min_id"));
    config->max_id = json_object_get_int(json_object_object_get(json, "max_id"));
    config->min_value = json_object_get_double(json_object_object_get(json, "min_value"));
    config->max_value = json_object_get_double(json_object_object_get(json, "max_value"));
    read_json_string(json, "unit", config->unit, sizeof(config->unit), "");
    config->update_interval_ms = json_object_get_int(json_object_object_get(json, "update_interval_ms"));

    json_object_put(json);
}

void load_config(const char* filename) {
    struct json_object* json = json_object_from_file(filename);
    if (!json) {
        fprintf(stderr, "Error: Could not load main config file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // Load main server config
    config.port = json_object_get_int(json_object_object_get(json, "port"));
    config.max_clients = json_object_get_int(json_object_object_get(json, "max_clients"));
    config.max_sensors = json_object_get_int(json_object_object_get(json, "max_sensors"));
    config.update_interval_ms = json_object_get_int(json_object_object_get(json, "update_interval_ms"));
    read_json_string(json, "log_file", config.log_file, sizeof(config.log_file), "telemetry.log");
    config.log_to_console = json_object_get_boolean(json_object_object_get(json, "log_to_console"));
    config.log_level = json_object_get_int(json_object_object_get(json, "log_level"));

    // Load sensor config paths
    struct json_object* sensor_configs;
    if (json_object_object_get_ex(json, "sensor_configs", &sensor_configs) &&
        json_object_get_type(sensor_configs) == json_type_array) {

        config.sensor_config_count = json_object_array_length(sensor_configs);
        config.sensor_configs = malloc(config.sensor_config_count * sizeof(SensorConfig));

        for (int i = 0; i < config.sensor_config_count; i++) {
            const char* config_path = json_object_get_string(json_object_array_get_idx(sensor_configs, i));
            load_sensor_config(config_path, &config.sensor_configs[i]);
        }
    }

    json_object_put(json);
}

const SensorConfig* get_sensor_config(const char* sensor_type) {
    for (int i = 0; i < config.sensor_config_count; i++) {
        if (strcmp(config.sensor_configs[i].sensor_type, sensor_type) == 0) {
            return &config.sensor_configs[i];
        }
    }
    return NULL;
}

const ServerConfig* get_config() {
    return &config;
}

void print_config() {
    const ServerConfig* cfg = get_config();

    printf("\n=== Server Configuration ===\n");
    printf("Port: %d\n", cfg->port);
    printf("Max clients: %d\n", cfg->max_clients);
    printf("Max sensors per client: %d\n", cfg->max_sensors);
    printf("Update interval: %d ms\n", cfg->update_interval_ms);
    printf("Log file: %s\n", cfg->log_file);
    printf("Log to console: %s\n", cfg->log_to_console ? "yes" : "no");
    printf("Log level: %d\n", cfg->log_level);

    printf("\n=== Sensor Configurations ===\n");
    for (int i = 0; i < cfg->sensor_config_count; i++) {
        const SensorConfig* sensor = &cfg->sensor_configs[i];
        printf("\nSensor type: %s\n", sensor->sensor_type);
        printf("ID range: %d - %d\n", sensor->min_id, sensor->max_id);
        printf("Value range: %.2f - %.2f %s\n",
              sensor->min_value, sensor->max_value, sensor->unit);
        printf("Update interval: %d ms\n", sensor->update_interval_ms);
    }
    printf("\n===========================\n");
}