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
        SerializeFormat format = get_random_serialization_format();
        serialize_data(&telemetry, buffer, sizeof(buffer), format);

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

void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    const ServerConfig* cfg = get_config();
    char client_ip[INET_ADDRSTRLEN];
    char buffer[1024];
    int* selected_sensors = malloc(cfg->max_sensors * sizeof(int));
    int sensor_count = 0;
    pthread_t* sensor_threads = NULL;

    if (!selected_sensors) {
        inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
        log_message(LOG_ERROR, "Memory allocation failed for client %s:%d",
                   client_ip, ntohs(client->address.sin_port));
        goto cleanup;
    }

    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->address.sin_port);
    log_message(LOG_INFO, "Client %s:%d connected", client_ip, client_port);

    // Формируем список датчиков
    char sensor_list[2048];
    snprintf(sensor_list, sizeof(sensor_list),
             "Available sensors (select up to %d comma-separated numbers):\n",
             cfg->max_sensors);

    for (int i = 0; i < cfg->sensor_config_count; i++) {
        char tmp[100];
        snprintf(tmp, sizeof(tmp), "%d. %s (IDs %d-%d)\n",
                i+1, cfg->sensor_configs[i].sensor_type,
                cfg->sensor_configs[i].min_id, cfg->sensor_configs[i].max_id);
        strncat(sensor_list, tmp, sizeof(sensor_list) - strlen(sensor_list) - 1);
    }
    strncat(sensor_list, "To exit press 'Ctrl+c'\nEnter your selection (e.g. '1,3,5'): ",
           sizeof(sensor_list) - strlen(sensor_list) - 1);

    // Отправляем список датчиков клиенту
    if (send(client->socket, sensor_list, strlen(sensor_list), 0) <= 0) {
        log_message(LOG_ERROR, "Failed to send sensor list to %s:%d", client_ip, client_port);
        goto cleanup;
    }

    // Получаем выбор клиента
    ssize_t bytes_read = recv(client->socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            log_message(LOG_INFO, "Client %s:%d disconnected during selection",
                       client_ip, client_port);
        } else {
            log_message(LOG_ERROR, "Error receiving selection from %s:%d: %s",
                       client_ip, client_port, strerror(errno));
        }
        goto cleanup;
    }

    buffer[bytes_read] = '\0';

    // Парсим выбор клиента
    char* token = strtok(buffer, ",");
    while (token != NULL && sensor_count < cfg->max_sensors) {
        int idx = atoi(token) - 1;  // Конвертируем в 0-based индекс
        if (idx >= 0 && idx < cfg->sensor_config_count) {
            selected_sensors[sensor_count++] = idx;
            log_message(LOG_INFO, "Client %s:%d selected sensor: %s",
                       client_ip, client_port,
                       cfg->sensor_configs[idx].sensor_type);
        }
        token = strtok(NULL, ",");
    }

    if (sensor_count == 0) {
        const char* msg = "No valid sensors selected. Disconnecting.\n";
        send(client->socket, msg, strlen(msg), 0);
        log_message(LOG_WARNING, "Client %s:%d selected no valid sensors",
                   client_ip, client_port);
        goto cleanup;
    }

    // Подтверждаем выбор
    const char* confirm_fmt = "Selected %d sensors. Starting data stream...\n";
    char confirm_msg[100];
    snprintf(confirm_msg, sizeof(confirm_msg), confirm_fmt, sensor_count);
    if (send(client->socket, confirm_msg, strlen(confirm_msg), 0) <= 0) {
        log_message(LOG_ERROR, "Failed to send confirmation to %s:%d",
                   client_ip, client_port);
        goto cleanup;
    }

    // Создаем массив для хранения ID потоков датчиков
    sensor_threads = malloc(sensor_count * sizeof(pthread_t));
    if (!sensor_threads) {
        log_message(LOG_ERROR, "Memory allocation failed for sensor threads");
        goto cleanup;
    }

    // Создаем потоки для каждого выбранного датчика
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

        if (pthread_create(&sensor_threads[i], NULL, sensor_thread, (void*)thread_data) != 0) {
            log_message(LOG_ERROR, "Thread creation failed for sensor %s", sensor_name);
            free(thread_data);
        }
    }

    // Основной цикл - проверяем соединение с клиентом
    while (server_running && is_client_connected(client->socket)) {
        sleep(1);
    }

    // Останавливаем все потоки датчиков
    for (int i = 0; i < sensor_count; i++) {
        if (sensor_threads[i]) {
            pthread_cancel(sensor_threads[i]);
        }
    }

    log_message(LOG_INFO, "Client %s:%d disconnected", client_ip, client_port);

cleanup:
    free(selected_sensors);
    if (sensor_threads) free(sensor_threads);
    close(client->socket);
    pthread_mutex_lock(&client_count_mutex);
    active_clients_count--;
    pthread_mutex_unlock(&client_count_mutex);
    free(client);
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
