cmake_minimum_required(VERSION 3.10)

project(rip VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic -Wunused -Wcast-align -fsanitize=undefined")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    message(STATUS "Adding debug flags to targets")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_subdirectory(tests)
endif()

add_subdirectory(librip)

add_executable(rip
    ripd.c
)

add_executable(rip-cli
    rip_cli.c
)

target_link_libraries(rip
    librip
)

target_link_libraries(rip-cli
    librip
)

# Installation
install(TARGETS rip DESTINATION bin)
install(TARGETS rip-cli DESTINATION bin)

