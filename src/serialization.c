#include "../include/serialization.h"
#include <stdio.h>
#include <string.h>

void serialize_to_json(const TelemetryData* data, char* buffer, size_t size) {
    if (strlen(data->sensor_type) == 0) {
        snprintf(buffer, size, "{\"error\":\"Invalid sensor data\"}\n");
        return;
    }

    snprintf(buffer, size,
        "{\"type\":\"%s\", \"id\":%d, \"value\":%s, \"unit\":\"%s\", \"time\":\"%s\"}\n",
        data->sensor_type,
        data->sensor_id,
        data->value_str,
        data->unit,
        data->timestamp
    );
}