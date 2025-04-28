#include "../include/config.h"
#include "../include/server.h"
#include "../include/logger.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void handle_shutdown_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    stop_server();
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    const char* config_file = (argc > 1) ? argv[1] : "../config.json";

    signal(SIGINT, handle_shutdown_signal);
    signal(SIGTERM, handle_shutdown_signal);
    setup_signal_handlers();

    load_config(config_file);
    print_config();

    start_server();
    run_server();

    return EXIT_SUCCESS;
}