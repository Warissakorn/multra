# The composed project schema's tests (included from cmake/cli_tests.cmake inside BUILD_TESTING).
add_test(NAME project-schema-cli COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:multra-project-schema>"
    -P "${PROJECT_SOURCE_DIR}/tests/project_schema_cli.cmake")
# D574: every slice's labelled cases against the emitted schema (tools/check_schema_labels.py). It
# needs Python's jsonschema 4.x; without it the test is not registered and configure says so.
# Linux CI sets MULTRA_REQUIRE_SCHEMA_LABELS so a missing jsonschema fails configure instead of skipping.
option(MULTRA_REQUIRE_SCHEMA_LABELS "Fail configure when the schema-labels test cannot be registered" OFF)
find_package(Python3 COMPONENTS Interpreter QUIET)
set(MULTRA_SCHEMA_LABELS_PYTHON "")
if(Python3_Interpreter_FOUND)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import jsonschema,sys; sys.exit(0 if jsonschema.Draft202012Validator else 1)"
        RESULT_VARIABLE multra_jsonschema_result OUTPUT_QUIET ERROR_QUIET)
    if(multra_jsonschema_result EQUAL 0)
        set(MULTRA_SCHEMA_LABELS_PYTHON "${Python3_EXECUTABLE}")
    endif()
endif()
if(MULTRA_SCHEMA_LABELS_PYTHON AND NOT MULTRA_SANITIZERS) # the label run is the same code; ASan would only slow it
    add_test(NAME schema-labels COMMAND "${MULTRA_SCHEMA_LABELS_PYTHON}" "${PROJECT_SOURCE_DIR}/tools/check_schema_labels.py"
        "$<TARGET_FILE:multra-tests>" "$<TARGET_FILE:multra-project-schema>" "${PROJECT_SOURCE_DIR}")
    set_tests_properties(schema-labels PROPERTIES TIMEOUT 900)
elseif(MULTRA_REQUIRE_SCHEMA_LABELS AND NOT MULTRA_SANITIZERS)
    message(FATAL_ERROR "MULTRA_REQUIRE_SCHEMA_LABELS is on but Python's jsonschema (Draft 2020-12) was not found")
else()
    message(STATUS "schema-labels not registered (needs Python jsonschema; skipped under sanitizers)")
endif()
