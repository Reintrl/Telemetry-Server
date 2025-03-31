#include "server.h"
#include "config.h"
#include "logger.h"
#include "data_generator.h"

int main(int argc, char** argv) {
    load_config("config.json");
    init_data_generator();
    start_server(get_config_port());
    handle_clients();
    return 0;
}