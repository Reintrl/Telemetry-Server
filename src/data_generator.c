#include "../include/data_generator.h"
#include <stdlib.h>
#include <time.h>

void init_data_generator() {
    srand(time(NULL));
}

TelemetryData generate_telemetry() {
    return (TelemetryData){
        .sensor_id = rand() % 10,
        .temperature = (rand() % 5000) / 100.0f,
        .status = "OK", // Реализуйте генерацию строк
        .timestamp = time(NULL)
    };
}