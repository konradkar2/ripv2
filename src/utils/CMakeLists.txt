cmake_minimum_required(VERSION 3.10)

project(utils VERSION 1.0)

add_library(${PROJECT_NAME}
        STATIC
        vector.c
)
add_library(rip::${PROJECT_NAME} ALIAS utils)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
)

