#include "../include/server.h"
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/data_generator.h"
#include "../include/serialization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
int active_clients_count = 0;

static int server_fd;
static volatile sig_atomic_t server_running = 1;

void handle_sigpipe(int sig) {
    log_message(LOG_INFO, "SIGPIPE received, client disconnected");
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = handle_sigpipe;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sa, NULL);
}

int is_client_connected(int sock) {
    char buf;
    int error = 0;
    socklen_t len = sizeof(error);

    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return 0;
    }

    if (recv(sock, &buf, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
        return 0;
    }

    return 1;
}

void* sensor_thread(void* arg) {
    sensor_thread_data_t* data = (sensor_thread_data_t*)arg;
    const SensorConfig* sensor_cfg = get_sensor_config(data->sensor_type);
    if (!sensor_cfg) {
        free(data);
        return NULL;
    }

    char buffer[512];
    struct timespec sleep_time = {
        .tv_sec = sensor_cfg->update_interval_ms / 1000,
        .tv_nsec = (sensor_cfg->update_interval_ms % 1000) * 1000000
    };

    while (server_running && is_client_connected(data->socket)) {
        TelemetryData telemetry = generate_telemetry_by_type(data->sensor_type);
        serialize_data(&telemetry, buffer, sizeof(buffer), data->format);  // Используем выбранный формат

        if (send(data->socket, buffer, strlen(buffer), MSG_NOSIGNAL) <= 0) {
            if (errno == EPIPE) {
                log_message(LOG_INFO, "Client %s:%d disconnected (broken pipe)",
                           data->client_ip, data->client_port);
            } else {
                log_message(LOG_ERROR, "Error sending to %s:%d: %s",
                           data->client_ip, data->client_port, strerror(errno));
            }
            break;
        }

        nanosleep(&sleep_time, NULL);
    }

    free(data);
    return NULL;
}

static void cleanup_client(client_info_t* client, int* selected_sensors,
                         pthread_t* sensor_threads, int sensor_count) {
    if (selected_sensors) free(selected_sensors);

    if (sensor_threads) {
        for (int i = 0; i < sensor_count; i++) {
            if (sensor_threads[i]) {
                pthread_cancel(sensor_threads[i]);
            }
        }
        free(sensor_threads);
    }

    if (client) {
        close(client->socket);
        pthread_mutex_lock(&client_count_mutex);
        active_clients_count--;
        pthread_mutex_unlock(&client_count_mutex);
        free(client);
    }
}

static int process_client_input(client_info_t* client, const ServerConfig* cfg,
                              int* selected_sensors, int* sensor_count,
                              int* attempts_remaining) {
    char client_ip[INET_ADDRSTRLEN];
    char buffer[1024];
    int all_valid = 1;
    int temp_selected_sensors[cfg->max_sensors];

    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->address.sin_port);

    // Формируем список датчиков
    char sensor_list[2048];
    snprintf(sensor_list, sizeof(sensor_list),
            "Available sensors (select up to %d comma-separated numbers, 1-%d):\n",
            cfg->max_sensors, cfg->sensor_config_count);

    for (int i = 0; i < cfg->sensor_config_count; i++) {
        char tmp[100];
        snprintf(tmp, sizeof(tmp), "%d. %s (IDs %d-%d)\n",
                i+1, cfg->sensor_configs[i].sensor_type,
                cfg->sensor_configs[i].min_id, cfg->sensor_configs[i].max_id);
        strncat(sensor_list, tmp, sizeof(sensor_list) - strlen(sensor_list) - 1);
    }

    // Добавляем информацию о попытках
    char attempts_msg[100];
    snprintf(attempts_msg, sizeof(attempts_msg),
            "Attempts remaining: %d\n", *attempts_remaining);
    strncat(sensor_list, attempts_msg, sizeof(sensor_list) - strlen(sensor_list) - 1);

    strncat(sensor_list, "Enter your selection (e.g. '1,3,5'): ",
           sizeof(sensor_list) - strlen(sensor_list) - 1);

    // Отправляем список
    if (send(client->socket, sensor_list, strlen(sensor_list), 0) <= 0) {
        log_message(LOG_ERROR, "Failed to send sensor list to %s:%d", client_ip, client_port);
        return -1;
    }

    // Получаем выбор
    ssize_t bytes_read = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            log_message(LOG_INFO, "Client %s:%d disconnected during selection",
                       client_ip, client_port);
        } else {
            log_message(LOG_ERROR, "Error receiving selection from %s:%d: %s",
                       client_ip, client_port, strerror(errno));
        }
        return -1;
    }

    buffer[bytes_read] = '\0';
    all_valid = 1;
    int temp_count = 0;

    // Парсим выбор
    char* token = strtok(buffer, ",");
    while (token != NULL && temp_count < cfg->max_sensors) {
        int idx = atoi(token) - 1;

        if (idx >= 0 && idx < cfg->sensor_config_count) {
            temp_selected_sensors[temp_count++] = idx;
        } else {
            all_valid = 0;
            log_message(LOG_WARNING, "Client %s:%d selected invalid sensor index: %d",
                       client_ip, client_port, idx+1);
        }
        token = strtok(NULL, ",");
    }

    if (all_valid && temp_count > 0) {
        for (int i = 0; i < temp_count; i++) {
            selected_sensors[i] = temp_selected_sensors[i];
            log_message(LOG_INFO, "Client %s:%d selected sensor: %s",
                       client_ip, client_port,
                       cfg->sensor_configs[temp_selected_sensors[i]].sensor_type);
        }
        *sensor_count = temp_count;
        return 1;
    } else {
        (*attempts_remaining)--;
        if (*attempts_remaining > 0) {
            const char* msg = "Invalid selection. Please try again.\n";
            send(client->socket, msg, strlen(msg), 0);
            log_message(LOG_WARNING, "Client %s:%d made invalid selection (%d attempts left",
                       client_ip, client_port, *attempts_remaining);
            return 0;
        } else {
            const char* msg = "Maximum attempts reached. Disconnecting.\n";
            send(client->socket, msg, strlen(msg), 0);
            log_message(LOG_WARNING, "Client %s:%d disconnected due to invalid selection",
                       client_ip, client_port);
            return -1;
        }
    }
}

void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    const ServerConfig* cfg = get_config();
    int* selected_sensors = malloc(cfg->max_sensors * sizeof(int));
    int sensor_count = 0;
    pthread_t* sensor_threads = NULL;
    int attempts_remaining = 3;
    char client_ip[INET_ADDRSTRLEN];

    if (!selected_sensors) {
        inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
        log_message(LOG_ERROR, "Memory allocation failed for client %s:%d",
                  client_ip, ntohs(client->address.sin_port));
        cleanup_client(client, NULL, NULL, 0);
        return NULL;
    }

    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->address.sin_port);
    log_message(LOG_INFO, "Client %s:%d connected", client_ip, client_port);

    // Обработка выбора сенсоров
    int input_result;
    do {
        input_result = process_client_input(client, cfg, selected_sensors, &sensor_count, &attempts_remaining);
        if (input_result == -1) {
            cleanup_client(client, selected_sensors, NULL, 0);
            return NULL;
        }
    } while (input_result == 0);

    // Запрос формата сериализации
    const char* format_prompt =
        "\nSelect serialization format:\n"
        "1. JSON\n"
        "2. XML\n"
        "3. CSV\n"
        "Enter your choice (1-3): ";

    if (send(client->socket, format_prompt, strlen(format_prompt), 0) <= 0) {
        log_message(LOG_ERROR, "Failed to send format prompt to %s:%d", client_ip, client_port);
        cleanup_client(client, selected_sensors, NULL, 0);
        return NULL;
    }

    // Получаем выбор формата
    char format_choice[10];
    ssize_t bytes_read = recv(client->socket, format_choice, sizeof(format_choice) - 1, 0);
    if (bytes_read <= 0) {
        log_message(LOG_INFO, "Client %s:%d disconnected during format selection",
                   client_ip, client_port);
        cleanup_client(client, selected_sensors, NULL, 0);
        return NULL;
    }
    format_choice[bytes_read] = '\0';

    // Определяем выбранный формат
    SerializeFormat format;
    switch (atoi(format_choice)) {
        case 1: format = SERIALIZE_JSON; break;
        case 2: format = SERIALIZE_XML; break;
        case 3: format = SERIALIZE_CSV; break;
        default:
            format = SERIALIZE_JSON;
            const char* invalid_msg = "Invalid format selection. Using JSON as default.\n";
            send(client->socket, invalid_msg, strlen(invalid_msg), 0);
            break;
    }

    // Подтверждаем выбор
    const char* format_name = "";
    switch (format) {
        case SERIALIZE_JSON: format_name = "JSON"; break;
        case SERIALIZE_XML:  format_name = "XML";  break;
        case SERIALIZE_CSV:  format_name = "CSV";  break;
    }
    char format_confirm[100];
    snprintf(format_confirm, sizeof(format_confirm),
            "Selected format: %s. Starting data stream...\n", format_name);
    send(client->socket, format_confirm, strlen(format_confirm), 0);

    log_message(LOG_INFO, "Client %s:%d selected format: %s",
               client_ip, client_port, format_name);

    // Создаем потоки для датчиков
    sensor_threads = malloc(sensor_count * sizeof(pthread_t));
    if (!sensor_threads) {
        log_message(LOG_ERROR, "Memory allocation failed for sensor threads");
        cleanup_client(client, selected_sensors, NULL, 0);
        return NULL;
    }

    for (int i = 0; i < sensor_count; i++) {
        int sensor_idx = selected_sensors[i];
        const char* sensor_name = cfg->sensor_configs[sensor_idx].sensor_type;

        sensor_thread_data_t* thread_data = malloc(sizeof(sensor_thread_data_t));
        if (!thread_data) {
            log_message(LOG_ERROR, "Memory allocation failed for sensor thread data");
            continue;
        }

        thread_data->socket = client->socket;
        strncpy(thread_data->client_ip, client_ip, INET_ADDRSTRLEN);
        thread_data->client_port = client_port;
        thread_data->sensor_type = sensor_name;
        thread_data->format = format;

        if (pthread_create(&sensor_threads[i], NULL, sensor_thread, (void*)thread_data) != 0) {
            log_message(LOG_ERROR, "Thread creation failed for sensor %s", sensor_name);
            free(thread_data);
        }
    }

    // Основной цикл проверки соединения
    while (server_running && is_client_connected(client->socket)) {
        sleep(1);
    }

    log_message(LOG_INFO, "Client %s:%d disconnected", client_ip, client_port);
    cleanup_client(client, selected_sensors, sensor_threads, sensor_count);
    return NULL;
}

void start_server() {
    const ServerConfig* cfg = get_config();

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(cfg->port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_message(LOG_ERROR, "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        log_message(LOG_ERROR, "Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        log_message(LOG_ERROR, "Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, cfg->max_clients) < 0) {
        log_message(LOG_ERROR, "Listen failed");
        exit(EXIT_FAILURE);
    }

    log_message(LOG_INFO, "Server started on port %d", cfg->port);
}

void run_server() {
    const ServerConfig* cfg = get_config();

    while(server_running) {
        client_info_t* client = malloc(sizeof(client_info_t));
        if (!client) {
            log_message(LOG_ERROR, "Memory allocation failed");
            continue;
        }

        socklen_t addr_len = sizeof(client->address);
        client->socket = accept(server_fd, (struct sockaddr*)&(client->address), &addr_len);

        if (client->socket < 0) {
            if (errno != EINTR) {
                log_message(LOG_ERROR, "Accept failed");
            }
            free(client);
            continue;
        }

        pthread_mutex_lock(&client_count_mutex);
        if (active_clients_count >= cfg->max_clients) {
            pthread_mutex_unlock(&client_count_mutex);

            const char* msg = "Server is busy. Please try again later.\n";
            send(client->socket, msg, strlen(msg), MSG_NOSIGNAL);

            log_message(LOG_INFO, "Rejected client (max clients reached)");
            close(client->socket);
            free(client);
            continue;
        }

        active_clients_count++;
        pthread_mutex_unlock(&client_count_mutex);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client) != 0) {
            log_message(LOG_ERROR, "Thread creation failed");

            pthread_mutex_lock(&client_count_mutex);
            active_clients_count--;
            pthread_mutex_unlock(&client_count_mutex);

            close(client->socket);
            free(client);
            continue;
        }

        pthread_detach(thread_id);
    }
}

void stop_server() {
    server_running = 0;
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    log_message(LOG_INFO, "Server stopped gracefully");
}
