cmake_minimum_required(VERSION 3.10)

project(rip VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(librip)
add_subdirectory(utils)
#add_subdirectory(tests)

# Source files

add_executable(rip
    main.c
)
target_link_libraries(rip
    librip
)

# Installation
install(TARGETS rip DESTINATION bin)