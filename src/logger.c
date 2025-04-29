#include "../include/logger.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

static FILE* log_file = NULL;
static const char* log_level_names[] = {"ERROR", "WARN", "INFO", "DEBUG"};

void init_logger() {
    const ServerConfig* cfg = get_config();

    if (cfg->log_file[0] != '\0') {
        log_file = fopen(cfg->log_file, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", cfg->log_file);
        } else {
            chmod(cfg->log_file, 0644);
        }
    }
}

void log_message(LogLevel level, const char* format, ...) {
    const ServerConfig* cfg = get_config();
    if (level > cfg->log_level) return;

    va_list args;
    char buffer[1024];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    // Форматируем время
    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S] ", tm_info);

    // Добавляем уровень логирования
    size_t prefix_len = strlen(buffer);
    snprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, "%s: ", log_level_names[level]);

    // Обновляем длину префикса
    prefix_len = strlen(buffer);

    // Добавляем сообщение
    va_start(args, format);
    vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, format, args);
    va_end(args);

    if (cfg->log_to_console) {
        fprintf(stderr, "%s\n", buffer);
    }

    if (log_file) {
        fprintf(log_file, "%s\n", buffer);
        fflush(log_file);
    }
}

void close_logger() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}