cmake_minimum_required(VERSION 3.20)
project(TelemetryServer C)
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_C REQUIRED json-c)

add_executable(TelemetryServer main.c
        main.c
        config.c
        config.h
        data_generator.c
        data_generator.h
        logger.c
        logger.h
        serialization.c
        serialization.h
        server.c
        server.h
)

target_include_directories(TelemetryServer PRIVATE ${JSON_C_INCLUDE_DIRS})
target_link_libraries(TelemetryServer PRIVATE ${JSON_C_LIBRARIES})