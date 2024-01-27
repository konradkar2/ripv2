cmake_minimum_required(VERSION 3.10)

project(tests VERSION 1.0)

find_program(MEMORYCHECK_COMMAND valgrind REQUIRED)
message(NOTICE "MEMORYCHECK_COMMAND: ${MEMORYCHECK_COMMAND}")
set(MEMORYCHECK_COMMAND_OPTIONS  "--leak-check=full --error-exitcode=100")

include(CTest)
enable_testing()

add_executable(tests
    test.c
    main.c
    vector_test.c
    hashmap_test.c
    rip_handle_resp_test.c
    rip_db_test.c
    route_test.c
    parse_rip_config_test.c
    rip_common_test.c
)

add_test(libriptest tests COMMAND tests)

target_link_libraries(tests
    librip
)

