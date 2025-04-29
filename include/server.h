#ifndef SERVER_H
#define SERVER_H

#include <signal.h>
#include <netinet/in.h>

extern pthread_mutex_t client_count_mutex;
extern int active_clients_count;

typedef struct {
    int socket;
    struct sockaddr_in address;
} client_info_t;

void setup_signal_handlers();
void start_server();
void run_server();
void stop_server();

#endif