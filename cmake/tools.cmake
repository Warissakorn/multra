# Tool executables (D630): the CLI, the schema dump, benchmarks, fixture builders and evidence sweeps.
# None is a test; the ones a test runs are added to `check` in cmake/cli_tests.cmake.
add_executable(multra-cli tools/run_simulation.cpp tools/four_step_cli.cpp tools/dynamic_assignment_cli.cpp tools/consistency_cli.cpp)
target_link_libraries(multra-cli PRIVATE multra_project multra_options)
target_include_directories(multra-cli PRIVATE ${CMAKE_BINARY_DIR}/generated)
add_dependencies(multra-cli multra-build-info)
multra_runtime_data(multra-cli)
add_executable(multra-project-schema tools/project_schema.cpp)
target_link_libraries(multra-project-schema PRIVATE multra_project multra_options)
install(TARGETS multra-cli multra-project-schema RUNTIME DESTINATION bin)
# D529 (FCTL_REFERENCE): M6 S2-c's analytic reference; pure arithmetic, no engine.
add_executable(multra-fctl-reference tools/fctl_reference.cpp)
target_link_libraries(multra-fctl-reference PRIVATE multra_eval multra_options)

# Not a test and not in `check`: it prints tick rates, it does not assert them. It needs no Qt,
# so the engine stays measurable under the headless preset.
add_executable(multra-engine-benchmark tools/engine_benchmark.cpp)
target_link_libraries(multra-engine-benchmark PRIVATE multra_commands multra_options)
target_precompile_headers(multra-engine-benchmark PRIVATE <nlohmann/json.hpp>)
multra_runtime_data(multra-engine-benchmark)
add_executable(multra-validate-benchmark tools/validate_benchmark.cpp)
target_link_libraries(multra-validate-benchmark PRIVATE multra_commands multra_options)
target_precompile_headers(multra-validate-benchmark REUSE_FROM multra-engine-benchmark)
multra_runtime_data(multra-validate-benchmark)

add_executable(multra-node-detail-benchmark tools/node_detail_benchmark.cpp)
target_link_libraries(multra-node-detail-benchmark PRIVATE multra_commands multra_options)
target_precompile_headers(multra-node-detail-benchmark REUSE_FROM multra-engine-benchmark)

# Regenerates the committed four-leg fixture from its one source, tools/four_leg_network.hpp.
add_executable(multra-four-leg-fixture tools/four_leg_fixture.cpp)
target_link_libraries(multra-four-leg-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-four-leg-fixture PRIVATE <nlohmann/json.hpp>)
# Regenerates the M2.6 template (tools/m26_study_network.hpp); multra-count-study writes a studies/ project (D575).
add_executable(multra-m26-study tools/m26_study.cpp)
target_link_libraries(multra-m26-study PRIVATE multra_commands multra_options)
target_precompile_headers(multra-m26-study REUSE_FROM multra-four-leg-fixture)
add_executable(multra-count-study tools/count_study.cpp)
target_link_libraries(multra-count-study PRIVATE multra_commands multra_options)
include(cmake/right_of_way_fixtures.cmake) # the T-junction and the priority-rule lab
# The M3.2.7b diagnostic sweep: archived evidence, not a test, so not part of `check`.
add_executable(multra-t-junction-sweep tools/t_junction_sweep.cpp)
target_link_libraries(multra-t-junction-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-t-junction-sweep REUSE_FROM multra-four-leg-fixture)
# M3.3.3a BA27: W74/prototype queue discharge at three timesteps (evidence, not part of `check`).
add_executable(multra-w74-discharge-sweep tools/w74_discharge_sweep.cpp)
target_link_libraries(multra-w74-discharge-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-w74-discharge-sweep REUSE_FROM multra-four-leg-fixture)
add_executable(multra-w99-discharge-sweep tools/w99_discharge_sweep.cpp)
target_link_libraries(multra-w99-discharge-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-w99-discharge-sweep REUSE_FROM multra-four-leg-fixture)
# M6 (D535, D538; docs/reference/M6_S1_RUN.md, M6_S2_RUN.md): the S1 project's builder and the run tools.
add_executable(multra-m6-s1-fixture tools/m6_s1_fixture.cpp)
target_link_libraries(multra-m6-s1-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-m6-s1-fixture REUSE_FROM multra-four-leg-fixture)
foreach(tool m6_s1_run m6_s2_run m6_u_run)
    string(REPLACE "_" "-" name "multra-${tool}")
    add_executable(${name} tools/${tool}.cpp)
    target_link_libraries(${name} PRIVATE multra_commands multra_runner multra_options)
    target_precompile_headers(${name} REUSE_FROM multra-four-leg-fixture)
    target_include_directories(${name} PRIVATE ${CMAKE_BINARY_DIR}/generated)
    add_dependencies(${name} multra-build-info)
endforeach()
# BA38 reaction-time sensitivity. Raw evidence tool; never run the 240-run matrix in check.
add_executable(multra-reaction-discharge-sweep tools/reaction_discharge_sweep.cpp)
target_link_libraries(multra-reaction-discharge-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-reaction-discharge-sweep REUSE_FROM multra-four-leg-fixture)
target_include_directories(multra-reaction-discharge-sweep PRIVATE ${CMAKE_BINARY_DIR}/generated)
add_dependencies(multra-reaction-discharge-sweep multra-build-info)
add_executable(multra-w74-clamp-trace tools/w74_clamp_trace.cpp)
target_link_libraries(multra-w74-clamp-trace PRIVATE multra_commands multra_options)
target_precompile_headers(multra-w74-clamp-trace REUSE_FROM multra-four-leg-fixture)
# D105: clamp traces and checked 40-seed evidence, not part of check or an M5 batch API.
add_executable(multra-t-junction-clamps tools/t_junction_clamps.cpp)
target_link_libraries(multra-t-junction-clamps PRIVATE multra_commands multra_options)
target_precompile_headers(multra-t-junction-clamps REUSE_FROM multra-four-leg-fixture)
# D98: the lane-change lab, regenerated from tools/lane_change_network.hpp, and its sweep
# (development evidence, not part of `check`).
add_executable(multra-lane-change-fixture tools/lane_change_fixture.cpp)
target_link_libraries(multra-lane-change-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-lane-change-fixture REUSE_FROM multra-four-leg-fixture)
add_executable(multra-lane-change-sweep tools/lane_change_sweep.cpp)
target_link_libraries(multra-lane-change-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-lane-change-sweep REUSE_FROM multra-four-leg-fixture)
# D449: the weaving lab, regenerated from tools/weaving_network.hpp, and its sweep (development
# evidence, not part of `check`).
add_executable(multra-weaving-fixture tools/weaving_fixture.cpp)
target_link_libraries(multra-weaving-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-weaving-fixture REUSE_FROM multra-four-leg-fixture)
add_executable(multra-weaving-sweep tools/weaving_sweep.cpp)
target_link_libraries(multra-weaving-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-weaving-sweep REUSE_FROM multra-four-leg-fixture)
# The Macro example (four-step procedure), regenerated from tools/macro_example_network.hpp.
add_executable(multra-macro-example tools/macro_example.cpp)
target_link_libraries(multra-macro-example PRIVATE multra_commands multra_options)
target_precompile_headers(multra-macro-example REUSE_FROM multra-four-leg-fixture)
# D500: the Node analysis example (LOS and the HCM table), regenerated from tools/node_example_network.hpp.
add_executable(multra-node-example tools/node_example.cpp)
target_link_libraries(multra-node-example PRIVATE multra_commands multra_options)
target_precompile_headers(multra-node-example REUSE_FROM multra-four-leg-fixture)
# D457: the lateral lab, regenerated from tools/lateral_lab_network.hpp (it reads data/ for the catalogs).
add_executable(multra-lateral-fixture tools/lateral_lab_fixture.cpp)
target_link_libraries(multra-lateral-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-lateral-fixture REUSE_FROM multra-four-leg-fixture)
