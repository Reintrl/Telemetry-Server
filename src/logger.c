#include "../include/logger.h"
#include <stdio.h>
#include <time.h>

void log_message(const char* message) {
    time_t now = time(NULL);
    fprintf(stderr, "[%lu] INFO: %s\n", now, message);
}

void log_error(const char* error) {
    time_t now = time(NULL);
    fprintf(stderr, "[%lu] ERROR: %s\n", now, error);
}