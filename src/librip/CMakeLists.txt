cmake_minimum_required(VERSION 3.10)

project(librip VERSION 1.0)

add_library(${PROJECT_NAME}
        STATIC
        rip.c
        rip_messages.c
        rip_if.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

