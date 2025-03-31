#include "../include/server.h"
#include "../include/data_generator.h"
#include "../include/serialization.h"
#include "../include/logger.h"
#include <string.h>          // Для strlen()
#include <stdlib.h>          // Для exit()
#include <stdio.h>           // Для printf()
#include <arpa/inet.h>  // Для inet_ntoa()
#include <netinet/in.h> // Для sockaddr_in


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
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

            if (new_socket < 0) {
                log_error("Accept failed");
                continue;
            }

            // Добавляем сокет в массив клиентов
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    char log_msg[100];
                    sprintf(log_msg, "New client connected: %s:%d",
                            inet_ntoa(client_addr.sin_addr),
                            ntohs(client_addr.sin_port));
                    log_message(log_msg);
                    break;
                }
            }
        }

        // Обход клиентов для отправки данных
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0 && FD_ISSET(client_sockets[i], &readfds)) {
                TelemetryData data = generate_telemetry();
                char json[256];
                serialize_to_json(&data, json, sizeof(json));

                if (send(client_sockets[i], json, strlen(json), 0) <= 0) {
                    close(client_sockets[i]);
                    client_sockets[i] = 0; // Удаляем сокет из массива
                    log_message("Client disconnected");
                }
            }
        }
    }
}