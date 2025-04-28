#ifndef SERVER_H
#define SERVER_H

#include <signal.h>

void setup_signal_handlers();
void start_server();
void run_server();
void stop_server();

#endif