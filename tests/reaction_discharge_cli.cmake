# An existing output directory is never reused, even when empty. Validate the forcing first.
set(output "${CMAKE_CURRENT_BINARY_DIR}/reaction-discharge-existing")
file(MAKE_DIRECTORY "${output}")
file(WRITE "${output}/sentinel.txt" "keep this file")
if(NOT IS_DIRECTORY "${output}")
    message(FATAL_ERROR "Failed to prepare existing output directory")
endif()
execute_process(COMMAND "${TOOL}" missing-project missing-preset missing-data missing-build "${output}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
file(READ "${output}/sentinel.txt" kept)
if(NOT result STREQUAL "1" OR NOT stderr MATCHES "Output already exists" OR NOT kept STREQUAL "keep this file")
    message(FATAL_ERROR "Output guard failed: ${result}, ${stderr}")
endif()
