#ifndef LOGGER_H
#define LOGGER_H

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

typedef enum {
    LOG_ERROR = 0,
    LOG_WARNING = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
} LogLevel;

void init_logger();
void log_message(LogLevel level, const char* format, ...);
void close_logger();

#endif