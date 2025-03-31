#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <sys/socket.h>      // Для sockaddr_in, socket(), etc
#include <netinet/in.h>      // Для INADDR_ANY, htons()
#include <unistd.h>          // Для close()

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void start_server();
void run_server();
void stop_server();
void set_server_port(int new_port);
void set_update_interval(int ms);

#endif