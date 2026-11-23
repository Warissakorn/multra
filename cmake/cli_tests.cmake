# Tests that run a tool, the CLI or a CMake script rather than a multra-tests group (D630),
# included inside BUILD_TESTING after cmake/model_tests.cmake.
add_test(NAME runtime-data-sync COMMAND "${CMAKE_COMMAND}" "-DTEST_ROOT=${CMAKE_BINARY_DIR}/runtime-data-sync-test"
    "-DSYNC_SCRIPT=${PROJECT_SOURCE_DIR}/cmake/sync_runtime_data.cmake"
    -P "${PROJECT_SOURCE_DIR}/tests/runtime_data_sync.cmake")
add_test(NAME architecture COMMAND multra-check-architecture "${PROJECT_SOURCE_DIR}")
add_test(NAME architecture-negative COMMAND multra-check-architecture --self-test)
add_test(NAME file-sizes COMMAND multra-check-file-sizes "${PROJECT_SOURCE_DIR}")
add_test(NAME documentation COMMAND multra-check-docs "${PROJECT_SOURCE_DIR}")
add_test(NAME documentation-negative COMMAND multra-check-docs --self-test)
add_test(NAME cli COMMAND multra-cli 42)
set_tests_properties(cli PROPERTIES PASS_REGULAR_EXPRESSION "31\\.75870")
add_test(NAME cli-invalid-seed COMMAND multra-cli 4294967296)
set_tests_properties(cli-invalid-seed PROPERTIES WILL_FAIL TRUE)
add_test(NAME reaction-discharge-help COMMAND multra-reaction-discharge-sweep --help)
add_test(NAME reaction-discharge-output-guard COMMAND "${CMAKE_COMMAND}"
    "-DTOOL=$<TARGET_FILE:multra-reaction-discharge-sweep>"
    -P "${PROJECT_SOURCE_DIR}/tests/reaction_discharge_cli.cmake")
include(cmake/schema_tests.cmake)
add_test(NAME cli-help COMMAND multra-cli --help)
add_executable(multra-study-area-cli-fixture tests/study_area_cli_fixture.cpp)
target_link_libraries(multra-study-area-cli-fixture PRIVATE multra_commands multra_options)
target_compile_definitions(multra-study-area-cli-fixture PRIVATE MULTRA_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME cli-study-area COMMAND "${CMAKE_COMMAND}"
    "-DCLI=$<TARGET_FILE:multra-cli>" "-DFIXTURE=$<TARGET_FILE:multra-study-area-cli-fixture>" "-DDATA=${PROJECT_SOURCE_DIR}/data" "-DTEST_ROOT=${CMAKE_BINARY_DIR}/study-area-cli-test"
    -P "${PROJECT_SOURCE_DIR}/tests/study_area_cli.cmake")
include(${PROJECT_SOURCE_DIR}/tests/consistency_cli_register.cmake)
# Exit code and message, not WILL_FAIL: a crash must not pass as a refusal.
add_test(NAME cli-phase-bin COMMAND "${CMAKE_COMMAND}"
    "-DCLI=$<TARGET_FILE:multra-cli>"
    "-DPROJECT=${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json"
    -P "${PROJECT_SOURCE_DIR}/tests/phase_bin_cli.cmake")
add_test(NAME cli-project COMMAND multra-cli 42 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
set_tests_properties(cli-project PROPERTIES PASS_REGULAR_EXPRESSION "North approach .* West exit")
add_test(NAME cli-batch COMMAND multra-cli --seeds 42-44 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
set_tests_properties(cli-batch PROPERTIES PASS_REGULAR_EXPRESSION "\"halfWidth95\": [0-9]")
add_test(NAME cli-batch-conflict COMMAND multra-cli --seed 1 --seeds 2-3 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
set_tests_properties(cli-batch-conflict PROPERTIES WILL_FAIL TRUE)
add_test(NAME cli-compare COMMAND multra-cli --seeds 42-43 --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json"
    --compare "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
set_tests_properties(cli-compare PROPERTIES PASS_REGULAR_EXPRESSION "\"difference\": 0\\.0")
add_test(NAME cli-compare-conflict COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json"
    --compare "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json")
set_tests_properties(cli-compare-conflict PROPERTIES PASS_REGULAR_EXPRESSION "--compare needs --project and --seeds")
# D528 (PRIORITY_LINES PL6): the priority-line counter on the T-junction, and its refusal.
add_test(NAME cli-priority-lines COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/data/projects/t-junction-priority.traffic.json" --priority-lines)
set_tests_properties(cli-priority-lines PROPERTIES PASS_REGULAR_EXPRESSION "\"priorityLines\".*\"control\": \"yield\".*\"siegloch\".*\"zone\": \"right-of-way/conflict-11\"")
add_test(NAME cli-priority-lines-refused COMMAND multra-cli 42 --priority-lines)
set_tests_properties(cli-priority-lines-refused PROPERTIES PASS_REGULAR_EXPRESSION "--priority-lines needs --project")
# D529 (FCTL_REFERENCE FC6): the S2-c reference script and its refusal.
add_test(NAME cli-fctl-reference COMMAND multra-fctl-reference --saturation 3600 --green-slots 5 --red-slots 5 --x 0.8)
set_tests_properties(cli-fctl-reference PROPERTIES PASS_REGULAR_EXPRESSION "\"slotSeconds\": 1\\.000000.*\"mu\": 0\\.400000.*\"meanOverflow\": 1\\.097.*\"meanDelaySlots\": 5\\.063.*\"referenceSeconds\": 5\\.063")
add_test(NAME cli-fctl-reference-refused COMMAND multra-fctl-reference --green-slots 5 --red-slots 5 --x 0.8)
set_tests_properties(cli-fctl-reference-refused PROPERTIES PASS_REGULAR_EXPRESSION "--saturation is required")
add_test(NAME cli-m6-s1-run-help COMMAND multra-m6-s1-run --help)
set_tests_properties(cli-m6-s1-run-help PROPERTIES PASS_REGULAR_EXPRESSION "usage: multra-m6-s1-run <project> <data-dir>")
add_test(NAME cli-m6-s1-run-refused COMMAND multra-m6-s1-run ${CMAKE_SOURCE_DIR}/data/projects/m6-s1-approach.traffic.json ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/data)
add_dependencies(check multra-reaction-discharge-sweep multra-project-schema multra-study-area-cli-fixture multra-fctl-reference multra-m6-s1-run multra-m6-s2-run multra-m6-u-run)
add_test(NAME cli-m6-s2-run-help COMMAND multra-m6-s2-run --help)
add_test(NAME cli-m6-s2-run-mode COMMAND multra-m6-s2-run x a b c)
set_tests_properties(cli-m6-s2-run-help cli-m6-s2-run-mode PROPERTIES PASS_REGULAR_EXPRESSION "usage: multra-m6-s2-run c\\|ab\\|arrivals\\|green <data-dir>")
add_test(NAME cli-m6-s2-run-refused COMMAND multra-m6-s2-run ab ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/data)
add_test(NAME cli-m6-u-run-help COMMAND multra-m6-u-run)
set_tests_properties(cli-m6-u-run-help PROPERTIES PASS_REGULAR_EXPRESSION "usage: multra-m6-u-run run\\|split <data-dir>")
add_test(NAME cli-m6-u-run-refused COMMAND multra-m6-u-run run ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/data)
set_tests_properties(cli-m6-s1-run-refused cli-m6-s2-run-refused cli-m6-u-run-refused PROPERTIES PASS_REGULAR_EXPRESSION "Output already exists")
add_test(NAME cli-m6-sweep-refused COMMAND multra-m6-u-run run ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/compile_commands.json x --candidate X)
add_test(NAME cli-m6-sweep-refused-mode COMMAND multra-m6-s2-run c ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/compile_commands.json x --candidate E)
set_tests_properties(cli-m6-sweep-refused cli-m6-sweep-refused-mode PROPERTIES PASS_REGULAR_EXPRESSION "Unknown candidate|--candidate needs mode ab")
# S2.3 (MACRO_NETWORK §9, D275): one period's static assignment, and its refusals.
add_test(NAME cli-macro COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --macro matrix:am)
add_test(NAME cli-street-segment-zone COMMAND "${CMAKE_COMMAND}" "-DCLI=$<TARGET_FILE:multra-cli>"
    "-DPROJECT=${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" "-DDATA=${PROJECT_SOURCE_DIR}/data"
    "-DTEST_ROOT=${CMAKE_BINARY_DIR}/street-segment-zone-cli-test" -P "${PROJECT_SOURCE_DIR}/tests/street_segment_zone_cli.cmake")
add_test(NAME cli-named-macro COMMAND "${CMAKE_COMMAND}"
    "-DCLI=$<TARGET_FILE:multra-cli>" "-DPROJECT=${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json"
    "-DDATA=${PROJECT_SOURCE_DIR}/data" "-DTEST_ROOT=${CMAKE_BINARY_DIR}/named-macro-cli-test"
    -P "${PROJECT_SOURCE_DIR}/tests/named_macro_cli.cmake")
set_tests_properties(cli-macro PROPERTIES PASS_REGULAR_EXPRESSION "\"converged\": true")
set(MULTRA_TG_CLI multra-cli --project "${PROJECT_SOURCE_DIR}/tests/trip-generation.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/tests/data/trip-generation")
add_test(NAME cli-trip-generation COMMAND ${MULTRA_TG_CLI} --trip-generation example-4-1)
set_tests_properties(cli-trip-generation PROPERTIES PASS_REGULAR_EXPRESSION "\"productions\": 5125")
add_test(NAME cli-trip-generation-conflict COMMAND ${MULTRA_TG_CLI} --trip-generation example-4-1 --macro matrix:am)
set_tests_properties(cli-trip-generation-conflict PROPERTIES PASS_REGULAR_EXPRESSION "--trip-generation needs --project and takes only --csv and --data-dir")
add_test(NAME cli-trip-generation-unknown COMMAND ${MULTRA_TG_CLI} --trip-generation none)
set_tests_properties(cli-trip-generation-unknown PROPERTIES PASS_REGULAR_EXPRESSION "TRIP_RATE_SET_UNKNOWN")
# M10.3 D1 (TRIP_DISTRIBUTION §8, D473): TD11.
set(MULTRA_TD_CLI multra-cli --project "${PROJECT_SOURCE_DIR}/tests/trip-generation.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution")
add_test(NAME cli-trip-distribution COMMAND ${MULTRA_TD_CLI} --trip-distribution one-way --deterrence hand)
add_test(NAME cli-trip-distribution-origin COMMAND ${MULTRA_TD_CLI} --trip-distribution one-way --deterrence hand-origin)
set_tests_properties(cli-trip-distribution-origin PROPERTIES PASS_REGULAR_EXPRESSION "\"constraint\": \"origin\"")
add_test(NAME cli-trip-distribution-destination COMMAND ${MULTRA_TD_CLI} --trip-distribution one-way --deterrence hand-destination)
set_tests_properties(cli-trip-distribution-destination PROPERTIES PASS_REGULAR_EXPRESSION "\"productionsModelled\"")
# PROCEDURE_SEQUENCE §4 (D484): PS9.
set(MULTRA_PS_CLI multra-cli --project "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution/procedure.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution")
add_test(NAME cli-procedure COMMAND ${MULTRA_PS_CLI} --procedure four-step)
add_test(NAME cli-procedure-unknown COMMAND ${MULTRA_PS_CLI} --procedure none)
add_test(NAME cli-procedure-feedback COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/data/procedure-feedback/example-11-3.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/tests/data/procedure-feedback" --procedure cw-005)
set_tests_properties(cli-procedure-feedback PROPERTIES PASS_REGULAR_EXPRESSION "\"converged\": true")
set_tests_properties(cli-procedure-unknown PROPERTIES PASS_REGULAR_EXPRESSION "PROCEDURE_UNKNOWN")
add_test(NAME cli-procedure-conflict COMMAND ${MULTRA_PS_CLI} --procedure four-step --macro period-am)
set_tests_properties(cli-procedure-conflict PROPERTIES PASS_REGULAR_EXPRESSION "--procedure needs --project and takes only --data-dir")
# D492: the shipped Macro example runs its four-step procedure on the installed data.
add_test(NAME cli-macro-example COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/data/projects/macro-four-step.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/data" --procedure four-step)
set_tests_properties(cli-procedure cli-macro-example PROPERTIES PASS_REGULAR_EXPRESSION "\"completed\": true")
add_test(NAME cli-node-example COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/data/projects/signalised-node-analysis.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/data" --node-analysis)
set_tests_properties(cli-node-example PROPERTIES PASS_REGULAR_EXPRESSION "\"name\": \"Example junction\"")
set(MULTRA_MC_CLI multra-cli --project "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution/mode-choice.traffic.json" --data-dir "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution")
add_test(NAME cli-mode-choice COMMAND ${MULTRA_MC_CLI} --trip-distribution one-way --deterrence hand --mode-choice hand --period period-am)
add_test(NAME cli-mode-choice-nested COMMAND ${MULTRA_MC_CLI} --trip-distribution one-way --deterrence hand --mode-choice nested --period period-am)
set_tests_properties(cli-mode-choice-nested PROPERTIES PASS_REGULAR_EXPRESSION "\"nests\": \\[")
add_test(NAME cli-mode-choice-alone COMMAND ${MULTRA_MC_CLI} --period period-am)
set_tests_properties(cli-mode-choice-alone PROPERTIES PASS_REGULAR_EXPRESSION "--period needs --mode-choice")
add_test(NAME cli-mode-choice-unknown COMMAND ${MULTRA_MC_CLI} --trip-distribution one-way --deterrence hand --mode-choice none)
set_tests_properties(cli-mode-choice-unknown PROPERTIES PASS_REGULAR_EXPRESSION "MODE_CHOICE_SET_UNKNOWN")
add_test(NAME cli-trip-distribution-conflict COMMAND ${MULTRA_TD_CLI} --trip-distribution one-way --deterrence hand --trip-generation one-way)
set_tests_properties(cli-trip-distribution-conflict PROPERTIES PASS_REGULAR_EXPRESSION "--trip-distribution needs --project and --deterrence")
add_test(NAME cli-trip-distribution-alone COMMAND ${MULTRA_TD_CLI} --deterrence hand)
set_tests_properties(cli-trip-distribution-alone PROPERTIES PASS_REGULAR_EXPRESSION "--deterrence, --skim-mode and --cost-matrix need --trip-distribution")
add_test(NAME cli-trip-distribution-unknown COMMAND ${MULTRA_TD_CLI} --trip-distribution one-way --deterrence none)
set_tests_properties(cli-trip-distribution-unknown PROPERTIES PASS_REGULAR_EXPRESSION "DISTRIBUTION_SET_UNKNOWN")
add_test(NAME cli-macro-seeds COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --macro matrix:am --seeds 1-2)
set_tests_properties(cli-macro-seeds PROPERTIES PASS_REGULAR_EXPRESSION "--macro needs --project and takes only --csv and --data-dir")
add_test(NAME cli-macro-period COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --macro pm)
set_tests_properties(cli-macro-period PROPERTIES PASS_REGULAR_EXPRESSION "MACRO_PERIOD_UNKNOWN")
# M11.4 (MESO_NETWORK §7, D300): the whole run on the Meso graph, and its refusals.
add_test(NAME cli-node-analysis COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/node-analysis-four-leg.traffic.json" --node-analysis)
set_tests_properties(cli-node-analysis PROPERTIES PASS_REGULAR_EXPRESSION "\"delay\": 59\\.86[0-9]*,\n *\"flow\": 2826\\.08")
add_test(NAME cli-node-analysis-args COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json" --node-analysis --seeds 1-2)
set_tests_properties(cli-node-analysis-args PROPERTIES PASS_REGULAR_EXPRESSION "--node-analysis needs --project and takes only --csv and --data-dir")
add_test(NAME cli-utdf COMMAND "${CMAKE_COMMAND}" "-DCLI=$<TARGET_FILE:multra-cli>" "-DSOURCE=${PROJECT_SOURCE_DIR}"
    "-DTEST_ROOT=${CMAKE_BINARY_DIR}/utdf-cli-test" -P "${PROJECT_SOURCE_DIR}/tests/utdf_cli.cmake")
add_test(NAME cli-meso COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/meso-two-route.traffic.json" --meso)
set_tests_properties(cli-trip-distribution cli-mode-choice cli-meso PROPERTIES PASS_REGULAR_EXPRESSION "\"validated\": false")
add_test(NAME cli-meso-seeds COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/meso-two-route.traffic.json" --meso --seeds 1-2)
set_tests_properties(cli-meso-seeds PROPERTIES PASS_REGULAR_EXPRESSION "--meso needs --project and takes only --csv and --data-dir")
add_test(NAME cli-meso-refused COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --meso)
set_tests_properties(cli-meso-refused PROPERTIES PASS_REGULAR_EXPRESSION "MESO_JAM_DENSITY_MISSING")
