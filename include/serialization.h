#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stddef.h>

#include "data_generator.h"

void serialize_to_json(const TelemetryData* data, char* buffer, size_t size);

#endif