#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../include/server.h"
#include "../include/data_generator.h"
#include "../include/serialization.h"
#include "../include/logger.h"

#define MAX_CLIENTS 10
#define DEFAULT_PORT 8080
#define DEFAULT_INTERVAL_MS 500

static int server_fd;
static int port = DEFAULT_PORT;
static int interval_ms = DEFAULT_INTERVAL_MS;

typedef struct {
    int socket;
    struct sockaddr_in address;
} client_info_t;

void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &(client->address.sin_addr), client_ip, INET_ADDRSTRLEN);
    log_message("Client connected: %s:%d", client_ip, ntohs(client->address.sin_port));

    while(1) {
        TelemetryData data = generate_telemetry();
        char json[256];
        serialize_to_json(&data, json, sizeof(json));

        if(send(client->socket, json, strlen(json), 0) <= 0) {
            break;
        }

        usleep(interval_ms * 1000);
    }

    close(client->socket);
    log_message("Client disconnected: %s:%d", client_ip, ntohs(client->address.sin_port));
    free(client);
    return NULL;
}

void start_server() {
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    log_message("Server started on port %d", port);
}

void run_server() {
    while(1) {
        client_info_t* client = malloc(sizeof(client_info_t));
        socklen_t addr_len = sizeof(client->address);

        client->socket = accept(server_fd, (struct sockaddr*)&(client->address), &addr_len);
        if (client->socket < 0) {
            perror("accept");
            free(client);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client) != 0) {
            perror("pthread_create");
            close(client->socket);
            free(client);
        }
        pthread_detach(thread_id);
    }
}

void stop_server() {
    close(server_fd);
    log_message("Server stopped");
}

void set_server_port(int new_port) {
    port = new_port;
}

void set_update_interval(int ms) {
    interval_ms = ms;
}