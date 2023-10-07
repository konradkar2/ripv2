cmake_minimum_required(VERSION 3.10)

project(tests VERSION 1.0)

find_program(MEMORYCHECK_COMMAND valgrind REQUIRED)
message(NOTICE "MEMORYCHECK_COMMAND: ${MEMORYCHECK_COMMAND}")
set(MEMORYCHECK_COMMAND_OPTIONS  "--trace-children=yes --leak-check=full --error-exitcode=100" )

include(CTest)
enable_testing()

add_executable(tests
    main.c
)

add_test(UTILS tests COMMAND tests)

#target_link_libraries(tests
#    librip
#)

