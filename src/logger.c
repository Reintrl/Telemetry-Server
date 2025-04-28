#include "../include/logger.h"
#include "../include/config.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

void log_message(const char* format, ...) {
    const ServerConfig* cfg = get_config();
    if (cfg->log_level < 2) return;

    va_list args;
    va_start(args, format);

    if (cfg->log_to_console) {
        time_t now = time(NULL);
        fprintf(stderr, "[%lu] INFO: ", now);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }

    va_end(args);
}

void log_error(const char* error) {
    time_t now = time(NULL);
    fprintf(stderr, "[%lu] ERROR: %s\n", now, error);
}