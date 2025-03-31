#include "../include/logger.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

void log_message(const char* format, ...) {
    time_t now = time(NULL);
    fprintf(stderr, "[%lu] INFO: ", now);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
}

void log_error(const char* error) {
    time_t now = time(NULL);
    fprintf(stderr, "[%lu] ERROR: %s\n", now, error);
}

