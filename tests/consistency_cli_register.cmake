# M12 R1 row SC7 (D553), included from the root CMakeLists beside cli-study-area.
add_executable(multra-consistency-cli-fixture ${PROJECT_SOURCE_DIR}/tests/consistency_cli_fixture.cpp)
target_link_libraries(multra-consistency-cli-fixture PRIVATE multra_commands multra_options)
target_compile_definitions(multra-consistency-cli-fixture PRIVATE MULTRA_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME cli-consistency COMMAND "${CMAKE_COMMAND}"
    "-DCLI=$<TARGET_FILE:multra-cli>" "-DFIXTURE=$<TARGET_FILE:multra-consistency-cli-fixture>"
    "-DDATA=${PROJECT_SOURCE_DIR}/data" "-DTEST_ROOT=${CMAKE_BINARY_DIR}/consistency-cli-test"
    -P "${PROJECT_SOURCE_DIR}/tests/consistency_cli.cmake")
