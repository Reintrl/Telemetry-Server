#include "../include/serialization.h"
#include <stdio.h>
#include <string.h>

void serialize_to_json(const TelemetryData* data, char* buffer, size_t size) {
    if (strlen(data->sensor_type) == 0) {
        snprintf(buffer, size, "{\"error\":\"Invalid sensor data\"}\n");
        return;
    }

    snprintf(buffer, size,
        "{\"type\":\"%s\", \"id\":%d, \"value\":%.2f, \"unit\":\"%s\", \"status\":\"%s\", \"time\":\"%s\"}\n",
        data->sensor_type,
        data->sensor_id,
        data->value,
        data->unit,
        data->status,
        data->timestamp
    );
}