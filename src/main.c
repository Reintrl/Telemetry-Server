#include "../include/server.h"
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/data_generator.h"

int main() {
    set_server_port(8080); // Опционально
    set_update_interval(1000); // 1 сек между отправками

    start_server();
    run_server(); // Блокирующий вызов
    // stop_server(); // По Ctrl+C
    return 0;
}