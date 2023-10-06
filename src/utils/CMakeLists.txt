cmake_minimum_required(VERSION 3.10)

project(utils VERSION 1.0)

add_library(${PROJECT_NAME}
        STATIC
        vector.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

