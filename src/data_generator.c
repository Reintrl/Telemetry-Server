#include "../include/data_generator.h"
#include "../include/config.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

void init_data_generator() {
    srand(time(NULL));
}

TelemetryData generate_telemetry() {
    const ServerConfig* cfg = get_config();
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    TelemetryData data;
    data.sensor_id = cfg->min_sensor_id + rand() % (cfg->max_sensor_id - cfg->min_sensor_id + 1);
    data.temperature = cfg->min_temperature +
                      (rand() % (int)((cfg->max_temperature - cfg->min_temperature) * 100)) / 100.0f;

    int status_index = rand() % 3;
    strncpy(data.status, cfg->status_options[status_index], sizeof(data.status)-1);
    data.status[sizeof(data.status)-1] = '\0';

    // Форматируем время в читаемый вид
    strftime(data.timestamp, sizeof(data.timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    return data;
}