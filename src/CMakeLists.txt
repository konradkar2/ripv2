cmake_minimum_required(VERSION 3.10)

project(rip VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wunused -Wcast-align -fsanitize=undefined")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTING "" OFF)
message(STATUS "BUILD_TESTING: " ${BUILD_TESTING})

if(BUILD_TESTING)
    set(CMAKE_BUILD_TYPE "Debug")
    message(STATUS "Adding debug flags to targets")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    add_subdirectory(tests)
endif()

add_subdirectory(librip)
add_subdirectory(utils)

add_executable(rip
    main.c
)
target_link_libraries(rip
    librip
)

# Installation
install(TARGETS rip DESTINATION bin)