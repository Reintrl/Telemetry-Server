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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

typedef struct {
    int socket;
    struct sockaddr_in address;
} client_info_t;

static int server_fd;
static volatile sig_atomic_t server_running = 1;

void handle_sigpipe(int sig) {
    log_message("SIGPIPE received, client disconnected");
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

    // Проверка ошибок сокета
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return 0;
    }

    // Проверка доступности чтения (без блокировки)
    if (recv(sock, &buf, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
        return 0;
    }

    return 1;
}

void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    const ServerConfig* cfg = get_config();
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    log_message("Client connected: %s:%d", client_ip, ntohs(client->address.sin_port));

    while(server_running && is_client_connected(client->socket)) {
        TelemetryData data = generate_telemetry();
        char buffer[256];

        serialize_to_json(&data, buffer, sizeof(buffer));

        ssize_t sent = send(client->socket, buffer, strlen(buffer), MSG_NOSIGNAL);
        if (sent <= 0) {
            if (errno != EPIPE) {
                log_error("Failed to send data to client");
            }
            break;
        }

        usleep(cfg->update_interval_ms * 1000);
    }

    close(client->socket);
    log_message("Client disconnected: %s:%d", client_ip, ntohs(client->address.sin_port));
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
        log_error("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        log_error("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        log_error("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, cfg->max_clients) < 0) {
        log_error("Listen failed");
        exit(EXIT_FAILURE);
    }

    log_message("Server started on port %d", cfg->port);
}

void run_server() {
    while(server_running) {
        client_info_t* client = malloc(sizeof(client_info_t));
        if (!client) {
            log_error("Memory allocation failed");
            continue;
        }

        socklen_t addr_len = sizeof(client->address);
        client->socket = accept(server_fd, (struct sockaddr*)&(client->address), &addr_len);

        if (client->socket < 0) {
            if (errno != EINTR) {
                log_error("Accept failed");
            }
            free(client);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client) != 0) {
            log_error("Thread creation failed");
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
    log_message("Server stopped gracefully");
}

