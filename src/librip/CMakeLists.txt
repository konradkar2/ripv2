cmake_minimum_required(VERSION 3.10)

project(librip VERSION 1.0)

find_package(PkgConfig REQUIRED)
find_library(LIBRT rt REQUIRED)
pkg_check_modules(LIBNL3 REQUIRED libnl-3.0 libnl-route-3.0 )

add_library(librip
        STATIC 
        utils/config/parse_rip_config.c
        utils/network/socket.c
        utils/hashmap.c
        utils/vector.c
        utils/timer.c
        utils/event_dispatcher.c
        utils/utils.c
        rip.c
        rip_recv.c
        rip_route.c
        rip_ipc.c
        rip_db.c
        rip_common.c
        rip_update.c
        rip_socket.c
)

target_link_libraries(librip PRIVATE
        ${LIBNL3_LIBRARIES}
        ${LIBRT}
        yaml
)

target_include_directories(librip PRIVATE
        ${LIBNL3_INCLUDE_DIRS}
)

target_include_directories(librip PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
