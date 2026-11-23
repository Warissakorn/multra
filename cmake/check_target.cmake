# The check tools and the `check` target (D630). Each test executable adds itself to `check`;
# cmake/check_builds_all.cmake adds every other executable at the end of the root file.
add_executable(multra-check-architecture tools/check_architecture.cpp)
add_executable(multra-check-file-sizes tools/check_file_sizes.cpp)
add_executable(multra-check-docs tools/check_docs.cpp)
target_link_libraries(multra-check-architecture PRIVATE multra_options)
target_link_libraries(multra-check-file-sizes PRIVATE multra_options)
target_link_libraries(multra-check-docs PRIVATE multra_options)
# ctest runs in parallel: the test groups below are separate ctest tests so that `-j` spreads
# them. Serial, the Debug desktop suite took 253 s; with 4 jobs 68 s (Linux, 2026-10-16).
# Sanitizer timing on hosted CI is too noisy to judge from one run: its check step took 8m12
# serial, 9m39 parallel and 13m18 serial (runs 554-556); locally it scaled 952 -> 254 s at 4 jobs.
include(ProcessorCount)
ProcessorCount(MULTRA_DETECTED_JOBS)
if(MULTRA_DETECTED_JOBS EQUAL 0)
    set(MULTRA_DETECTED_JOBS 2)
endif()
set(MULTRA_TEST_JOBS "${MULTRA_DETECTED_JOBS}" CACHE STRING "ctest jobs for the check target")
add_custom_target(check
    COMMAND $<TARGET_FILE:multra-check-architecture> "${PROJECT_SOURCE_DIR}"
    COMMAND $<TARGET_FILE:multra-check-file-sizes> "${PROJECT_SOURCE_DIR}"
    COMMAND $<TARGET_FILE:multra-check-docs> "${PROJECT_SOURCE_DIR}"
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${MULTRA_TEST_JOBS} -C $<CONFIG>
    DEPENDS multra-cli multra-check-architecture multra-check-file-sizes multra-check-docs
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
