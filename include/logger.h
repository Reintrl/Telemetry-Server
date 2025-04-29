#ifndef LOGGER_H
#define LOGGER_H

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