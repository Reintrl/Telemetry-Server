#include "../include/config.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

static void read_string_array(struct json_object* obj, const char* key,
                            char dest[][20], size_t count, const char default_val[][20]) {
    struct json_object* tmp;
    if (json_object_object_get_ex(obj, key, &tmp) &&
        json_object_get_type(tmp) == json_type_array) {

        size_t array_len = json_object_array_length(tmp);
        size_t to_copy = (array_len < count) ? array_len : count;

        for (size_t i = 0; i < to_copy; i++) {
            struct json_object* item = json_object_array_get_idx(tmp, i);
            const char* str = json_object_get_string(item);
            strncpy(dest[i], str, sizeof(dest[i]) - 1);
            dest[i][sizeof(dest[i]) - 1] = '\0';
        }
    } else if (default_val) {
        for (size_t i = 0; i < count; i++) {
            strncpy(dest[i], default_val[i], sizeof(dest[i]) - 1);
            dest[i][sizeof(dest[i]) - 1] = '\0';
        }
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
    read_string_array(json, "status_options", config->status_options, 3, NULL);
    config->update_variation = json_object_get_double(json_object_object_get(json, "update_variation"));

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
    config.max_sensors = json_object_get_int(json_object_object_get(json, "max_sensors"));  // Добавлено
    config.update_interval_ms = json_object_get_int(json_object_object_get(json, "update_interval_ms"));
    read_json_string(json, "log_file", config.log_file, sizeof(config.log_file), "telemetry.log");
    config.log_to_console = json_object_get_boolean(json_object_object_get(json, "log_to_console"));
    config.log_level = json_object_get_int(json_object_object_get(json, "log_level"));
    read_json_string(json, "serialization_format", config.serialization_format,
                   sizeof(config.serialization_format), "json");

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
    printf("Max sensors per client: %d\n", cfg->max_sensors);  // Добавлено
    printf("Update interval: %d ms\n", cfg->update_interval_ms);
    printf("Log file: %s\n", cfg->log_file);
    printf("Log to console: %s\n", cfg->log_to_console ? "yes" : "no");
    printf("Log level: %d\n", cfg->log_level);
    printf("Serialization format: %s\n", cfg->serialization_format);

    printf("\n=== Sensor Configurations ===\n");
    for (int i = 0; i < cfg->sensor_config_count; i++) {
        const SensorConfig* sensor = &cfg->sensor_configs[i];
        printf("\nSensor type: %s\n", sensor->sensor_type);
        printf("ID range: %d - %d\n", sensor->min_id, sensor->max_id);
        printf("Value range: %.2f - %.2f %s\n", sensor->min_value, sensor->max_value, sensor->unit);
        printf("Status options: %s, %s, %s\n",
              sensor->status_options[0], sensor->status_options[1], sensor->status_options[2]);
        printf("Update variation: %.2f\n", sensor->update_variation);
    }
    printf("\n===========================\n");
}