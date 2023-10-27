cmake_minimum_required(VERSION 3.10)

project(librip VERSION 1.0)

find_package(PkgConfig REQUIRED)
find_library(LIBRT rt REQUIRED)
pkg_check_modules(LIBNL3 REQUIRED libnl-3.0 libnl-route-3.0 )

add_library(librip
        STATIC 
        rip.c
        rip_messages.c
        rip_if.c
        rip_handle_resp.c
        rip_route.c
        rip_ipc.c
)

target_link_libraries(librip PRIVATE
        ${LIBNL3_LIBRARIES}
        ${LIBRT}
)

target_include_directories(librip PRIVATE
        ${LIBNL3_INCLUDE_DIRS}
)

target_include_directories(librip PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
