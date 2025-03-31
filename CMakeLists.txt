cmake_minimum_required(VERSION 3.20)
project(TelemetryServer C)
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_C REQUIRED json-c)

add_executable(TelemetryServer src/main.c
        src/main.c
        src/config.c
        include/config.h
        src/data_generator.c
        include/data_generator.h
        src/logger.c
        include/logger.h
        src/serialization.c
        include/serialization.h
        src/server.c
        include/server.h
)

target_include_directories(TelemetryServer PRIVATE ${JSON_C_INCLUDE_DIRS})
target_link_libraries(TelemetryServer PRIVATE ${JSON_C_LIBRARIES})