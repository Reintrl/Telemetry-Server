#include "../include/serialization.h"
#include <stdio.h>
#include <string.h>

void serialize_to_json(const TelemetryData* data, char* buffer, size_t size) {
    if (strlen(data->sensor_type) == 0) {
        snprintf(buffer, size, "{\"error\":\"Invalid sensor data\"}\n");
        return;
    }

    if (strcmp(data->sensor_type, "gps") == 0) {
        // Специальный формат для GPS данных
        snprintf(buffer, size,
            "{\"type\":\"%s\", \"id\":%d, \"coordinates\":\"%s\", \"unit\":\"%s\", \"status\":\"%s\", \"time\":\"%s\"}\n",
            data->sensor_type,
            data->sensor_id,
            data->value_str,
            data->unit,
            data->status,
            data->timestamp
        );
    } else {
        // Стандартный формат для других датчиков
        snprintf(buffer, size,
            "{\"type\":\"%s\", \"id\":%d, \"value\":%s, \"unit\":\"%s\", \"status\":\"%s\", \"time\":\"%s\"}\n",
            data->sensor_type,
            data->sensor_id,
            data->value_str,
            data->unit,
            data->status,
            data->timestamp
        );
    }
}