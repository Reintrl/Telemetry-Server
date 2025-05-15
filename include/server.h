#ifndef SERVER_H
#define SERVER_H

#include <signal.h>
#include <netinet/in.h>
#include "config.h"
#include "logger.h"
#include "data_generator.h"
#include "serialization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

extern pthread_mutex_t client_count_mutex;
extern int active_clients_count;

typedef struct {
    int socket;
    struct sockaddr_in address;
} client_info_t;

typedef struct {
    int socket;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
    const char* sensor_type;
    SerializeFormat format;
} sensor_thread_data_t;

void setup_signal_handlers();
void start_server();
void run_server();
void stop_server();

#endif