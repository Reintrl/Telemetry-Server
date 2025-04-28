#include "../include/config.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Инициализация конфигурации по умолчанию
static ServerConfig config = {
    // Server settings
    .port = 8080,
    .max_clients = 10,
    .update_interval_ms = 1000,

    // Data generator settings
    .min_sensor_id = 0,
    .max_sensor_id = 10,
    .min_temperature = -20.0f,
    .max_temperature = 50.0f,
    .status_options = {"OK", "WARNING", "CRITICAL"},

    // Logging settings
    .log_file = "telemetry_server.log",
    .log_to_console = 1,
    .log_level = 2, // INFO level by default

    // Serialization settings
    .serialization_format = "json"
};

// Внутренняя функция для чтения строки из JSON
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

// Внутренняя функция для чтения массива строк из JSON
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

void load_config(const char* filename) {
    struct json_object* json = json_object_from_file(filename);
    if (!json) {
        fprintf(stderr, "Warning: Could not load config file '%s' (%s). Using defaults.\n",
               filename, errno ? strerror(errno) : "file not found");
        return;
    }

    struct json_object* tmp;

    // Server settings
    if (json_object_object_get_ex(json, "port", &tmp))
        config.port = json_object_get_int(tmp);

    if (json_object_object_get_ex(json, "max_clients", &tmp))
        config.max_clients = json_object_get_int(tmp);

    if (json_object_object_get_ex(json, "update_interval_ms", &tmp))
        config.update_interval_ms = json_object_get_int(tmp);

    // Data generator settings
    if (json_object_object_get_ex(json, "min_sensor_id", &tmp))
        config.min_sensor_id = json_object_get_int(tmp);

    if (json_object_object_get_ex(json, "max_sensor_id", &tmp))
        config.max_sensor_id = json_object_get_int(tmp);

    if (json_object_object_get_ex(json, "min_temperature", &tmp))
        config.min_temperature = (float)json_object_get_double(tmp);

    if (json_object_object_get_ex(json, "max_temperature", &tmp))
        config.max_temperature = (float)json_object_get_double(tmp);

    read_string_array(json, "status_options", config.status_options,
                     sizeof(config.status_options)/sizeof(config.status_options[0]),
                     NULL);

    // Logging settings
    read_json_string(json, "log_file", config.log_file, sizeof(config.log_file), NULL);

    if (json_object_object_get_ex(json, "log_to_console", &tmp))
        config.log_to_console = json_object_get_boolean(tmp);

    if (json_object_object_get_ex(json, "log_level", &tmp))
        config.log_level = json_object_get_int(tmp);

    // Serialization settings
    read_json_string(json, "serialization_format", config.serialization_format,
                    sizeof(config.serialization_format), "json");

    json_object_put(json);
}

const ServerConfig* get_config() {
    return &config;
}

void print_config() {
    const ServerConfig* cfg = get_config();

    printf("\n=== Current Configuration ===\n");

    printf("\nServer Settings:\n");
    printf("  Port: %d\n", cfg->port);
    printf("  Max clients: %d\n", cfg->max_clients);
    printf("  Update interval: %d ms\n", cfg->update_interval_ms);

    printf("\nData Generator Settings:\n");
    printf("  Sensor ID range: %d - %d\n", cfg->min_sensor_id, cfg->max_sensor_id);
    printf("  Temperature range: %.2f - %.2f\n", cfg->min_temperature, cfg->max_temperature);
    printf("  Status options: ");
    for (size_t i = 0; i < sizeof(cfg->status_options)/sizeof(cfg->status_options[0]); i++) {
        printf("%s%s", cfg->status_options[i],
              (i < sizeof(cfg->status_options)/sizeof(cfg->status_options[0]) - 1 ? ", " : ""));
    }
    printf("\n");

    printf("\nLogging Settings:\n");
    printf("  Log file: %s\n", cfg->log_file);
    printf("  Log to console: %s\n", cfg->log_to_console ? "yes" : "no");
    printf("  Log level: %d (", cfg->log_level);
    switch(cfg->log_level) {
        case 0: printf("ERROR"); break;
        case 1: printf("WARNING"); break;
        case 2: printf("INFO"); break;
        case 3: printf("DEBUG"); break;
        default: printf("UNKNOWN");
    }
    printf(")\n");

    printf("\nSerialization Settings:\n");
    printf("  Format: %s\n", cfg->serialization_format);

    printf("\n============================\n\n");
}