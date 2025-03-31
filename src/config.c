#include "../include/config.h"
#include <stdio.h>
#include <json-c/json.h>

static int server_port = 8080;

void load_config(const char* filename) {
    struct json_object* config = json_object_from_file(filename);
    json_object_object_get_ex(config, "port", &server_port);
}

int get_config_port() {
    return server_port;
}