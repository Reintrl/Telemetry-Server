#include "server.h"
#include "data_generator.h"
#include "serialization.h"
#include "logger.h"
#include <string.h>          // Для strlen()
#include <stdlib.h>          // Для exit()
#include <stdio.h>           // Для printf()


static int server_fd;
static int client_sockets[MAX_CLIENTS];

void start_server(int port) {
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    // Создание и настройка сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    log_message("Server started");
}

void handle_clients() {
    fd_set readfds;
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        // Добавление клиентов в набор
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if(client_sockets[i] > 0) FD_SET(client_sockets[i], &readfds);
            if(client_sockets[i] > max_sd) max_sd = client_sockets[i];
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // Обработка новых подключений
        if(FD_ISSET(server_fd, &readfds)) {
            int new_socket = accept(server_fd, NULL, NULL);
            // Добавление в массив клиентов
        }

        // Обход клиентов для отправки данных
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if(FD_ISSET(client_sockets[i], &readfds)) {
                TelemetryData data = generate_telemetry();
                char buffer[256];
                serialize_to_json(&data, buffer, sizeof(buffer));
                send(client_sockets[i], buffer, strlen(buffer), 0);
            }
        }
    }
}