#include "../include/serialization.h"

#include <stdio.h>

void serialize_to_json(const TelemetryData* data, char* buffer, size_t size) {
    snprintf(buffer, size,
        "{\"sensor_id\":%d, \"temp\":%.2f, \"status\":\"%s\", \"ts\":%u}\n",  // Добавлен \n
        data->sensor_id,
        data->temperature,
        data->status,
        data->timestamp
    );
}
