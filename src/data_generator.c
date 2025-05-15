#include "../include/data_generator.h"

void init_data_generator() {
    srand(time(NULL));
}

TelemetryData generate_telemetry() {
    const ServerConfig* cfg = get_config();
    if (cfg->sensor_config_count == 0) {
        TelemetryData empty = {0};
        return empty;
    }

    int sensor_type_idx = rand() % cfg->sensor_config_count;
    return generate_telemetry_by_type(cfg->sensor_configs[sensor_type_idx].sensor_type);
}

TelemetryData generate_telemetry_by_type(const char* sensor_type) {
    const SensorConfig* sensor_cfg = get_sensor_config(sensor_type);
    if (!sensor_cfg) {
        TelemetryData empty = {0};
        return empty;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    TelemetryData data;
    strcpy(data.sensor_type, sensor_type);
    data.sensor_id = sensor_cfg->min_id + rand() % (sensor_cfg->max_id - sensor_cfg->min_id + 1);
    strcpy(data.unit, sensor_cfg->unit);

    if (strcmp(sensor_type, "gps") == 0) {
        double lat = 53.9 + (rand() % 1000) / 10000.0;
        double lon = 27.5667 + (rand() % 1000) / 10000.0;
        snprintf(data.value_str, sizeof(data.value_str), "%.6f,%.6f", lat, lon);
        data.value = 0.0;
    } else {
        data.value = sensor_cfg->min_value +
                   (rand() % (int)((sensor_cfg->max_value - sensor_cfg->min_value) * 100)) / 100.0;
        snprintf(data.value_str, sizeof(data.value_str), "%.2f", data.value);
    }

    strftime(data.timestamp, sizeof(data.timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    return data;
}

TelemetryData generate_telemetry_by_id(int sensor_id) {
    const ServerConfig* cfg = get_config();

    for (int i = 0; i < cfg->sensor_config_count; i++) {
        if (sensor_id >= cfg->sensor_configs[i].min_id &&
            sensor_id <= cfg->sensor_configs[i].max_id) {

            TelemetryData data = generate_telemetry_by_type(cfg->sensor_configs[i].sensor_type);
            data.sensor_id = sensor_id;
            return data;
        }
    }

    TelemetryData empty = {0};
    return empty;
}