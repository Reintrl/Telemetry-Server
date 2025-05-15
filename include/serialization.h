#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stddef.h>
#include "config.h"
#include "data_generator.h"
#include <stdio.h>
#include <string.h>

SerializeFormat get_random_serialization_format();
void serialize_data(const TelemetryData* data, char* buffer, size_t size, SerializeFormat format);
void serialize_to_json(const TelemetryData* data, char* buffer, size_t size);
void serialize_to_xml(const TelemetryData* data, char* buffer, size_t size);
void serialize_to_csv(const TelemetryData* data, char* buffer, size_t size);

#endif
