#include "../include/serialization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void serialize_data(const TelemetryData* data, char* buffer, size_t size, SerializeFormat format) {
    switch(format) {
        case SERIALIZE_JSON:
            serialize_to_json(data, buffer, size);
            break;
        case SERIALIZE_XML:
            serialize_to_xml(data, buffer, size);
            break;
        case SERIALIZE_CSV:
            serialize_to_csv(data, buffer, size);
            break;
        default:
            serialize_to_json(data, buffer, size);
    }
}

void serialize_to_json(const TelemetryData* data, char* buffer, size_t size) {
    if (strlen(data->sensor_type) == 0) {
        snprintf(buffer, size, "{\"error\":\"Invalid sensor data\"}\n");
        return;
    }

    snprintf(buffer, size,
        "{\"type\":\"%s\",\"id\":%d,\"value\":%s,\"unit\":\"%s\",\"time\":\"%s\"}\n",
        data->sensor_type,
        data->sensor_id,
        data->value_str,
        data->unit,
        data->timestamp
    );
}

void serialize_to_xml(const TelemetryData* data, char* buffer, size_t size) {
    if (strlen(data->sensor_type) == 0) {
        snprintf(buffer, size, "<error>Invalid sensor data</error>");
        return;
    }

    snprintf(buffer, size,
        "<telemetry>\n"
        "  <type>%s</type>\n"
        "  <id>%d</id>\n"
        "  <value>%s</value>\n"
        "  <unit>%s</unit>\n"
        "  <time>%s</time>\n"
        "</telemetry>\n",
        data->sensor_type,
        data->sensor_id,
        data->value_str,
        data->unit,
        data->timestamp
    );
}

void serialize_to_csv(const TelemetryData* data, char* buffer, size_t size) {
    if (strlen(data->sensor_type) == 0) {
        snprintf(buffer, size, "error,Invalid sensor data");
        return;
    }

    snprintf(buffer, size,
        "%s,%d,%s,%s,%s\n",
        data->sensor_type,
        data->sensor_id,
        data->value_str,
        data->unit,
        data->timestamp
    );
}