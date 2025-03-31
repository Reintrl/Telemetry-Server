#ifndef LOGGER_H
#define LOGGER_H

void log_message(const char* format, ...);  // Добавляем поддержку variadic arguments
void log_error(const char* error);

#endif