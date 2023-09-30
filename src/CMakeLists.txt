cmake_minimum_required(VERSION 3.10)

project(rip VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Source files
set(SOURCES
    main.c
    rip.c
    rip_ifc.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define the executable
add_executable(rip ${SOURCES})

# Installation
install(TARGETS rip DESTINATION bin)