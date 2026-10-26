cmake_minimum_required(VERSION 3.24)
project(MULTRA VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# D287: these options were TRAFFICSIM_* before the rename. A script or cache still passing the
# old name keeps working, with a warning, unless the new name is also given.
foreach(name BUILD_DESKTOP SANITIZERS LLD TEST_JOBS)
    if(DEFINED TRAFFICSIM_${name} AND NOT DEFINED MULTRA_${name})
        message(DEPRECATION "TRAFFICSIM_${name} is now MULTRA_${name}")
        set(MULTRA_${name} "${TRAFFICSIM_${name}}")
    endif()
endforeach()
option(MULTRA_BUILD_DESKTOP "Build the Qt Widgets desktop harness" ON)
option(MULTRA_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
include(CTest)

add_library(multra_options INTERFACE)
target_compile_definitions(multra_options INTERFACE
    MULTRA_VERSION="${PROJECT_VERSION}"
    MULTRA_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
if(MSVC)
    target_compile_options(multra_options INTERFACE /W4 /permissive- /utf-8 /fp:strict)
else()
    target_compile_options(multra_options INTERFACE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off
        # Aggregates are built with trailing members left to their defaults on purpose; the
        # warning for that was 1120 of 1151 lines and hid the ones that mean something.
        -Wno-missing-field-initializers)
    if(MULTRA_SANITIZERS)
        target_compile_options(multra_options INTERFACE -fsanitize=address,undefined,float-cast-overflow
            -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(multra_options INTERFACE -fsanitize=address,undefined,float-cast-overflow)
    endif()
    # Touching one source relinks some 40 executables, and GNU ld made that most of an incremental
    # build: measured 2026-10-09, multra-tests linked in 4.5 s with ld and 0.57 s with lld.
    # Global, so every executable gets it; a toolchain without lld keeps its default linker.
    option(MULTRA_LLD "Link with lld when the compiler accepts it" ON)
    if(MULTRA_LLD)
        include(CheckLinkerFlag)
        check_linker_flag(CXX -fuse-ld=lld MULTRA_HAS_LLD)
        if(MULTRA_HAS_LLD)
            add_link_options(-fuse-ld=lld)
        endif()
    endif()
endif()

# System/vcpkg packages; no network downloads happen implicitly during configure.
find_package(nlohmann_json 3.11 CONFIG QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_path(MULTRA_JSON_INCLUDE_DIR nlohmann/json.hpp REQUIRED)
    add_library(multra_json INTERFACE)
    target_include_directories(multra_json SYSTEM INTERFACE "${MULTRA_JSON_INCLUDE_DIR}")
    add_library(nlohmann_json::nlohmann_json ALIAS multra_json)
endif()

add_library(multra_core STATIC
    src/macro/graph.cpp src/macro/assign.cpp src/meso/graph.cpp src/meso/demand.cpp src/meso/node_model.cpp src/meso/link_model.cpp src/meso/loading.cpp
    src/core/demand.cpp src/core/following.cpp src/core/routes.cpp src/core/speed_decision.cpp src/core/speed_area.cpp
    src/core/signals.cpp src/core/red_running.cpp src/core/caps.cpp src/core/join_anticipation.cpp src/core/simulation.cpp src/core/validate.cpp src/core/conflicts.cpp
    src/core/lanes.cpp src/core/routing.cpp src/core/w74.cpp src/core/perception.cpp src/core/lateral.cpp src/core/lateral_motion.cpp src/core/leaders.cpp src/core/speed_control.cpp src/core/parking.cpp src/core/retune.cpp)
target_link_libraries(multra_core PRIVATE multra_options)

add_library(multra_model STATIC src/model/network/geometry.cpp
    src/model/network/connector_equation.cpp src/model/network/compile.cpp src/model/network/validate.cpp src/model/network/connector_geometry.cpp
    src/model/network/diagnostics.cpp src/model/network/connector_paths.cpp src/model/network/road_boundaries.cpp src/model/network/connector_surface.cpp
    src/model/network/sections.cpp src/model/network/markings.cpp src/model/network/rotation.cpp src/model/network/head_slots.cpp src/model/demand/signal_control.cpp src/model/demand/macro_compile.cpp src/model/demand/meso_compile.cpp src/model/demand/coarse_compile.cpp src/model/demand/seam_checks.cpp src/model/demand/meso_paths.cpp src/model/demand/meso_road_paths.cpp
    src/model/network/routing.cpp src/model/network/lane_family.cpp src/model/network/routeless.cpp src/model/network/right_of_way.cpp src/model/network/travel_time.cpp src/model/network/polygon.cpp src/model/network/nodes.cpp src/model/network/roads.cpp src/model/network/road_crossings.cpp src/model/network/coarse_nodes.cpp src/model/network/node_detail.cpp src/model/network/study_area.cpp src/model/network/node_membership.cpp src/model/network/node_suggestions.cpp
    src/model/network/conflict_surface.cpp src/model/network/conflict_groups.cpp src/model/network/conflict_coverage.cpp src/model/network/conflict_crossings.cpp src/model/network/conflict_polygons.cpp
    src/model/network/conflict_polygon_math.cpp
    src/model/network/conflict_bands.cpp
    src/model/network/conflict_mouth_bands.cpp src/model/network/automatic_conflicts.cpp)
target_link_libraries(multra_model PUBLIC multra_core PRIVATE multra_options)
# Run-view geometry is Qt-free and independently testable; the core never imports it.
add_library(multra_vehicle_pose STATIC src/editor/vehicle_pose.cpp src/editor/rear_axle_pose.cpp src/editor/lane_change_pose.cpp)
target_link_libraries(multra_vehicle_pose PUBLIC multra_model PRIVATE multra_options)
add_library(multra_eval STATIC src/eval/discharge.cpp src/eval/discharge_passage.cpp src/eval/summary.cpp src/eval/movement.cpp src/eval/nodes.cpp src/eval/sections.cpp src/eval/los.cpp src/eval/lane_changes.cpp src/eval/segment_times.cpp src/eval/stop_lines.cpp src/eval/arrival_phases.cpp src/eval/dead_end_waits.cpp src/eval/lots.cpp)
target_link_libraries(multra_eval PUBLIC multra_core PRIVATE multra_options)
# M5.2 batches: independent seeds and their aggregate. Core and eval only (check_architecture).
add_library(multra_runner STATIC src/runner/batch.cpp src/runner/compare.cpp)
target_link_libraries(multra_runner PUBLIC multra_eval PRIVATE multra_options)
add_library(multra_project STATIC src/project/load.cpp src/project/parse.cpp src/project/output.cpp
    src/project/document.cpp src/project/signal_migration.cpp src/project/definition.cpp src/project/run.cpp src/project/display.cpp src/project/diagnostics.cpp
    src/project/node_codec.cpp src/project/study_period_codec.cpp src/project/study_area_codec.cpp src/project/study_area_run.cpp src/project/study_area_macro.cpp src/project/study_area_meso.cpp src/project/side_files.cpp src/project/export_marking.cpp src/project/mode_codec.cpp src/project/link_type_codec.cpp src/project/road_codec.cpp src/project/macro_input.cpp src/project/macro_settings.cpp src/project/meso_settings.cpp src/project/meso_input.cpp src/project/meso_run.cpp src/project/readiness.cpp src/project/node_proposals.cpp src/project/meso_output.cpp src/project/macro_run.cpp src/project/macro_output.cpp src/project/input_sha256.cpp src/project/input_manifest.cpp src/project/behaviour_library.cpp src/project/discharge_json.cpp src/project/evaluation.cpp src/project/demand_paths.cpp src/project/demand_preview.cpp src/project/demand_catalog.cpp src/project/demand_time_types.cpp src/project/station_routing.cpp src/project/batch_output.cpp src/project/node_output.cpp src/project/comparison_output.cpp src/project/evaluation_period.cpp src/project/clamp_rules.cpp src/project/los_output.cpp src/project/counted_volumes.cpp src/project/map_codec.cpp src/project/route_destination.cpp src/project/speed_decisions.cpp src/project/speed_areas.cpp src/project/speed_curve.cpp src/project/lane_preference.cpp src/project/zone_codec.cpp src/project/od_demand.cpp src/project/parking_compile.cpp
    src/project/parallel_seeds.cpp)
# D152: the seed scheduler's worker threads live here; runner stays thread-free (check_architecture).
find_package(Threads REQUIRED)
target_sources(multra_project PRIVATE src/project/macro_run_definition.cpp src/project/observed_schema.cpp src/project/document_envelope.cpp)
target_sources(multra_project PRIVATE src/project/meso_road_demand.cpp)
target_link_libraries(multra_project PUBLIC multra_model multra_eval multra_runner Threads::Threads
    nlohmann_json::nlohmann_json PRIVATE multra_options)

# nlohmann/json is a large header-only template library: 1.47 s to parse and instantiate here,
# against 0.36 s for <nlohmann/json_fwd.hpp>. src/project/json.hpp includes the forward
# declaration, so only a translation unit that builds or reads a Json pays for the definition,
# and precompiling it on the multi-source targets turns that into one cost per target. A
# single-source target gets no PCH of its own -- it would cost about what it saves -- but it can
# REUSE_FROM one that already exists, which src/shell does for the UI test executables.
target_precompile_headers(multra_project PRIVATE <nlohmann/json.hpp>)

add_library(multra_commands STATIC src/commands/history.cpp src/commands/network_commands.cpp
    src/commands/split_link.cpp src/commands/connector_commands.cpp src/commands/delete_objects.cpp
    src/commands/demand_commands.cpp src/commands/catalog_commands.cpp src/commands/road_commands.cpp src/commands/behaviour_commands.cpp src/commands/appearance_commands.cpp src/commands/link_geometry_commands.cpp
    src/commands/right_of_way_commands.cpp src/commands/conflict_authoring.cpp
    src/commands/node_commands.cpp src/commands/coarse_commands.cpp src/commands/fill_commands.cpp src/commands/node_detail_commands.cpp src/commands/conflict_follow.cpp src/commands/conflict_group_commands.cpp src/commands/map_commands.cpp src/commands/zone_commands.cpp)
target_link_libraries(multra_commands PUBLIC multra_project PRIVATE multra_options)
target_sources(multra_commands PRIVATE src/commands/macro_run_commands.cpp)
target_precompile_headers(multra_commands PRIVATE <nlohmann/json.hpp>)
# D165 behaviour lab: the ring, its run and live retune. Qt-free; the shell draws it (check_architecture).
add_library(multra_lab STATIC src/lab/lab_settings.cpp src/lab/lab_network.cpp src/lab/lab_oval.cpp src/lab/lab_urban.cpp src/lab/lab_run.cpp)
target_link_libraries(multra_lab PUBLIC multra_commands PRIVATE multra_options)

function(multra_runtime_data target)
    add_dependencies(${target} multra-data)
endfunction()
# Mirror on every build, including deletions. PROJ is managed by multra-proj-data below.
add_custom_target(multra-data ALL
    COMMAND ${CMAKE_COMMAND} "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}/data"
        "-DDESTINATION_DIR=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,$<CONFIG>/,>data"
        "-DBUILD_DIR=${CMAKE_BINARY_DIR}" -DPRESERVE_PATHS=proj
        -P "${PROJECT_SOURCE_DIR}/cmake/sync_runtime_data.cmake"
    VERBATIM)

# The commit the CLI reports with batch output; regenerated on every build, rewritten only on change.
add_custom_target(multra-build-info
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${PROJECT_SOURCE_DIR} -DOUTPUT=${CMAKE_BINARY_DIR}/generated/build_info.hpp
            -P ${PROJECT_SOURCE_DIR}/cmake/build_info.cmake
    BYPRODUCTS ${CMAKE_BINARY_DIR}/generated/build_info.hpp)
add_executable(multra-cli tools/run_simulation.cpp)
target_link_libraries(multra-cli PRIVATE multra_project multra_options)
target_include_directories(multra-cli PRIVATE ${CMAKE_BINARY_DIR}/generated)
add_dependencies(multra-cli multra-build-info)
multra_runtime_data(multra-cli)
add_executable(multra-project-schema tools/project_schema.cpp)
target_link_libraries(multra-project-schema PRIVATE multra_project multra_options)
install(TARGETS multra-cli multra-project-schema RUNTIME DESTINATION bin)
install(DIRECTORY data/ DESTINATION bin/data)

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
# Regenerates the M2.6 study template from its one source, tools/m26_study_network.hpp.
add_executable(multra-m26-study tools/m26_study.cpp)
target_link_libraries(multra-m26-study PRIVATE multra_commands multra_options)
target_precompile_headers(multra-m26-study REUSE_FROM multra-four-leg-fixture)
# Regenerates the M3.2.7 T-junction from its one source, tools/t_junction_network.hpp.
add_executable(multra-t-junction-fixture tools/t_junction_fixture.cpp)
target_link_libraries(multra-t-junction-fixture PRIVATE multra_commands multra_options)
target_precompile_headers(multra-t-junction-fixture REUSE_FROM multra-four-leg-fixture)
# The M3.2.7b diagnostic sweep: archived evidence, not a test, so not part of `check`.
add_executable(multra-t-junction-sweep tools/t_junction_sweep.cpp)
target_link_libraries(multra-t-junction-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-t-junction-sweep REUSE_FROM multra-four-leg-fixture)
# M3.3.3a BA27: W74/prototype queue discharge at three timesteps (evidence, not part of `check`).
add_executable(multra-w74-discharge-sweep tools/w74_discharge_sweep.cpp)
target_link_libraries(multra-w74-discharge-sweep PRIVATE multra_commands multra_options)
target_precompile_headers(multra-w74-discharge-sweep REUSE_FROM multra-four-leg-fixture)
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

if(BUILD_TESTING)
    add_executable(multra-lane-change-pose-tests tests/lane_change_pose_tests.cpp)
    target_link_libraries(multra-lane-change-pose-tests PRIVATE multra_vehicle_pose multra_options)
    add_test(NAME lane-change-pose COMMAND multra-lane-change-pose-tests)
    add_dependencies(check multra-lane-change-pose-tests)
    add_executable(multra-rear-axle-pose-tests tests/rear_axle_pose_tests.cpp)
    target_link_libraries(multra-rear-axle-pose-tests PRIVATE multra_vehicle_pose multra_options)
    add_test(NAME rear-axle-pose COMMAND multra-rear-axle-pose-tests)
    add_dependencies(check multra-rear-axle-pose-tests)
    add_executable(multra-vehicle-pose-tests tests/vehicle_pose_tests.cpp)
    target_link_libraries(multra-vehicle-pose-tests PRIVATE multra_vehicle_pose multra_options)
    add_test(NAME vehicle-pose COMMAND multra-vehicle-pose-tests)
    add_dependencies(check multra-vehicle-pose-tests)
    add_executable(multra-tests tests/main.cpp tests/behaviour_library_tests.cpp tests/behaviour_selection_tests.cpp tests/behaviour_selection_edge_tests.cpp tests/input_manifest_tests.cpp tests/discharge_tests.cpp tests/discharge_passage_tests.cpp tests/discharge_identity_tests.cpp tests/core_tests.cpp tests/station_routing_tests.cpp tests/source_insertion_tests.cpp
        tests/network_tests.cpp tests/reference_tests.cpp tests/project_tests.cpp tests/editor_model_tests.cpp tests/authoring_tests.cpp
        tests/network_lifecycle_tests.cpp tests/connector_tests.cpp tests/connector_shape_tests.cpp tests/connector_mouth_tests.cpp tests/connector_point_tests.cpp tests/attachment_tests.cpp tests/lane_edge_tests.cpp tests/diagnostics_tests.cpp tests/demand_tests.cpp tests/demand_correctness_tests.cpp tests/demand_catalog_tests.cpp tests/demand_time_types_tests.cpp tests/range_tests.cpp tests/four_leg_tests.cpp tests/m26_study_tests.cpp tests/right_of_way_tests.cpp tests/right_of_way_lifecycle_tests.cpp tests/right_of_way_resolution_tests.cpp tests/conflict_zone_tests.cpp tests/conflict_chain_tests.cpp tests/commitment_tests.cpp tests/lane_change_tests.cpp tests/stop_control_tests.cpp tests/stop_control_model_tests.cpp tests/signal_position_tests.cpp tests/queue_counter_tests.cpp tests/t_junction_tests.cpp tests/t_junction_controlled_tests.cpp tests/t_junction_signal_tests.cpp tests/automatic_conflict_tests.cpp tests/conflict_geometry_tests.cpp tests/conflict_polygon_tests.cpp tests/conflict_mouth_group_tests.cpp tests/conflict_follow_tests.cpp tests/right_of_way_runtime_tests.cpp tests/right_of_way_editor_tests.cpp tests/demand_interval_tests.cpp tests/demand_composition_tests.cpp tests/demand_decision_tests.cpp tests/movement_tests.cpp tests/arrival_phase_tests.cpp tests/wait_cause_tests.cpp tests/routeless_tests.cpp tests/downstream_decision_tests.cpp tests/decision_interval_tests.cpp tests/demand_counts_tests.cpp tests/signal_control_tests.cpp tests/discretionary_tests.cpp tests/lane_change_lab_tests.cpp tests/batch_tests.cpp tests/compare_tests.cpp tests/evaluation_period_tests.cpp tests/cooldown_tests.cpp tests/amber_tests.cpp tests/travel_time_section_tests.cpp tests/los_tests.cpp tests/w74_tests.cpp tests/w74_codec_tests.cpp tests/perception_codec_tests.cpp tests/perception_motion_tests.cpp tests/perception_lane_tests.cpp tests/perception_delay_tests.cpp tests/reaction_discharge_tests.cpp tests/lateral_codec_tests.cpp tests/lateral_span_tests.cpp tests/lateral_motion_tests.cpp tests/lateral_zone_tests.cpp tests/lane_change_duration_tests.cpp tests/lane_swap_tests.cpp tests/straddler_leader_tests.cpp tests/curve_speed_tests.cpp tests/kerb_bias_tests.cpp tests/commitment_deceleration_tests.cpp tests/merge_deceleration_tests.cpp tests/speed_decision_tests.cpp tests/speed_area_tests.cpp tests/speed_distribution_tests.cpp tests/stop_dwell_tests.cpp tests/lane_preference_tests.cpp tests/driver_factor_tests.cpp tests/red_running_tests.cpp tests/merge_clamp_tests.cpp tests/join_anticipation_tests.cpp tests/stop_floor_tests.cpp tests/node_tests.cpp tests/node_membership_tests.cpp tests/node_evaluation_tests.cpp tests/node_suggestion_tests.cpp tests/study_period_tests.cpp tests/mode_pcu_tests.cpp tests/macro_network_tests.cpp tests/meso_network_tests.cpp tests/macro_assign_tests.cpp tests/macro_benchmark_tests.cpp tests/meso_demand_tests.cpp tests/meso_node_model_tests.cpp tests/meso_loading_tests.cpp tests/meso_output_tests.cpp tests/macro_output_tests.cpp tests/road_catalog_tests.cpp tests/road_tests.cpp tests/road_crossing_tests.cpp tests/road_coarse_tests.cpp tests/road_coarse_command_tests.cpp tests/road_seam_tests.cpp tests/readiness_tests.cpp tests/fill_defaults_tests.cpp tests/node_detail_tests.cpp tests/study_area_tests.cpp tests/study_area_marking_tests.cpp tests/node_proposal_tests.cpp tests/give_way_stop_tests.cpp tests/w74_traits_tests.cpp tests/w74_run_tests.cpp)
    target_link_libraries(multra-tests PRIVATE multra_lab multra_commands multra_options)
    target_sources(multra-tests PRIVATE tests/observed_schema_tests.cpp tests/project_schema_tests.cpp tests/project_envelope_tests.cpp tests/evaluation_review_tests.cpp tests/study_area_vehicle_type_tests.cpp tests/macro_run_definition_tests.cpp tests/road_zone_tests.cpp tests/meso_road_demand_tests.cpp)
    target_compile_definitions(multra-tests PRIVATE MULTRA_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    # Release spent 77 % of a test file's compile in -O3 optimisation (2026-10-13). The test code
    # itself needs no -O3; the libraries it calls keep it, so the engine under test is unchanged.
    target_compile_options(multra-tests PRIVATE $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-O1>)
    target_precompile_headers(multra-tests PRIVATE <nlohmann/json.hpp>)
    target_sources(multra-tests PRIVATE tests/route_destination_tests.cpp tests/map_project_tests.cpp tests/zones_tests.cpp tests/od_demand_tests.cpp tests/parking_lot_tests.cpp tests/parking_project_tests.cpp tests/parking_output_tests.cpp tests/seed_mixing_tests.cpp tests/circuit_tests.cpp tests/lab_tests.cpp tests/history_tests.cpp tests/rotation_tests.cpp tests/connector_surface_tests.cpp tests/lane_correspondence_tests.cpp tests/connector_mouth_sweep_tests.cpp tests/connector_lane_centre_tests.cpp tests/connector_equation_tests.cpp tests/connector_edit_motion_tests.cpp)
    # One ctest test per group, so `ctest -j` spreads them; a single unfiltered run was 60 s of
    # serial wall time and repeated every named group.
    set(MULTRA_TEST_GROUPS mapproject behaviourlibrary behaviourselection provenance discharge core stationrouting network reference project editor connectors mouths points
        diagnostics demand ranges attachments authoring lifecycle history rotation fourleg movement
        routeless signals m26study m26study_merge rightofway rightofway_lifecycle rightofway_resolution
        conflict_zone conflict_chain commitment stop_control stop_control_model signal_position
        queue_counter tjunction tjunction_controlled tjunction_signal automatic_conflict conflict_geometry
        rightofway_runtime rightofway_editor lanechange lane_correspondence mouth_sweep conflict_follow
        discretionary lanelab lane_centres equation batch compare evaluationperiod cooldown amber traveltime los w74 w74codec perceptioncodec perceptionmotion perceptionlane perceptiondelay reactiondischarge lateralcodec lateralspan lateralmotion lateralzone lanechangeduration laneswap straddlerleader curvespeed kerbbias commitdecel mergedecel speeddecision speedarea speeddist stopdwell lanepref driverfactor redrunning mergeclamps joinanticipation stopfloor nodes nodemembership nodeevaluation nodesuggest studyperiods modepcu macronetwork mesonetwork macroassign macrobenchmark mesodemand mesonode mesoloading mesooutput macrooutput roadcatalog roads roadcrossings roadcoarse roadseams readiness filldefaults nodedetail studyarea nodeproposals givewaystop w74traits w74run demandcounts zonescodec zonescommands odexpand parkinglot parkingcodec parkingproject parkingoutputs seedmixing circuit lab)
    list(APPEND MULTRA_TEST_GROUPS namedmacro roadzone mesoroad)
    foreach(group ${MULTRA_TEST_GROUPS})
        add_test(NAME ${group} COMMAND multra-tests ${group})
    endforeach()
    # A newly registered group must never silently disappear from CI just because the list
    # above was not updated (previously "points"): this fails on any group it does not name.
    add_test(NAME all-model-tests COMMAND multra-tests --check-groups ${MULTRA_TEST_GROUPS})
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
    add_dependencies(check multra-reaction-discharge-sweep)
    add_test(NAME project-schema-cli COMMAND "${CMAKE_COMMAND}"
        "-DTOOL=$<TARGET_FILE:multra-project-schema>"
        -P "${PROJECT_SOURCE_DIR}/tests/project_schema_cli.cmake")
    add_dependencies(check multra-project-schema)
    add_test(NAME cli-help COMMAND multra-cli --help)
    add_executable(multra-study-area-cli-fixture tests/study_area_cli_fixture.cpp)
    target_link_libraries(multra-study-area-cli-fixture PRIVATE multra_commands multra_options)
    target_compile_definitions(multra-study-area-cli-fixture PRIVATE MULTRA_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME cli-study-area COMMAND "${CMAKE_COMMAND}"
        "-DCLI=$<TARGET_FILE:multra-cli>" "-DFIXTURE=$<TARGET_FILE:multra-study-area-cli-fixture>"
        "-DDATA=${PROJECT_SOURCE_DIR}/data" "-DTEST_ROOT=${CMAKE_BINARY_DIR}/study-area-cli-test"
        -P "${PROJECT_SOURCE_DIR}/tests/study_area_cli.cmake")
    add_dependencies(check multra-study-area-cli-fixture)
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
    # S2.3 (MACRO_NETWORK §9, D275): one period's static assignment, and its refusals.
    add_test(NAME cli-macro COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --macro matrix:am)
    add_test(NAME cli-road-zone COMMAND "${CMAKE_COMMAND}" "-DCLI=$<TARGET_FILE:multra-cli>"
        "-DPROJECT=${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" "-DDATA=${PROJECT_SOURCE_DIR}/data"
        "-DTEST_ROOT=${CMAKE_BINARY_DIR}/road-zone-cli-test" -P "${PROJECT_SOURCE_DIR}/tests/road_zone_cli.cmake")
    add_test(NAME cli-named-macro COMMAND "${CMAKE_COMMAND}"
        "-DCLI=$<TARGET_FILE:multra-cli>" "-DPROJECT=${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json"
        "-DDATA=${PROJECT_SOURCE_DIR}/data" "-DTEST_ROOT=${CMAKE_BINARY_DIR}/named-macro-cli-test"
        -P "${PROJECT_SOURCE_DIR}/tests/named_macro_cli.cmake")
    set_tests_properties(cli-macro PROPERTIES PASS_REGULAR_EXPRESSION "\"converged\": true")
    add_test(NAME cli-macro-seeds COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --macro matrix:am --seeds 1-2)
    set_tests_properties(cli-macro-seeds PROPERTIES PASS_REGULAR_EXPRESSION "--macro needs --project and takes only --csv and --data-dir")
    add_test(NAME cli-macro-period COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --macro pm)
    set_tests_properties(cli-macro-period PROPERTIES PASS_REGULAR_EXPRESSION "MACRO_PERIOD_UNKNOWN")
    # M11.4 (MESO_NETWORK §7, D300): the whole run on the Meso graph, and its refusals.
    add_test(NAME cli-meso COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/meso-two-route.traffic.json" --meso)
    set_tests_properties(cli-meso PROPERTIES PASS_REGULAR_EXPRESSION "\"validated\": false")
    add_test(NAME cli-meso-seeds COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/meso-two-route.traffic.json" --meso --seeds 1-2)
    set_tests_properties(cli-meso-seeds PROPERTIES PASS_REGULAR_EXPRESSION "--meso needs --project and takes only --csv and --data-dir")
    add_test(NAME cli-meso-refused COMMAND multra-cli --project "${PROJECT_SOURCE_DIR}/tests/macro-two-route.traffic.json" --meso)
    set_tests_properties(cli-meso-refused PROPERTIES PASS_REGULAR_EXPRESSION "MESO_JAM_DENSITY_MISSING")
    add_dependencies(check multra-tests)
endif()

if(MULTRA_BUILD_DESKTOP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network)
    find_package(PROJ CONFIG REQUIRED)
    add_library(multra_geo STATIC src/geo/projection.cpp src/geo/tile_grid.cpp src/geo/projection_data.cpp)
    target_link_libraries(multra_geo PUBLIC multra_project PRIVATE PROJ::proj multra_options)
    if(BUILD_TESTING)
        add_executable(multra-map-geo-tests tests/map_geo_tests.cpp)
        target_link_libraries(multra-map-geo-tests PRIVATE multra_geo multra_options)
        add_test(NAME map-geo COMMAND multra-map-geo-tests)
        add_dependencies(check multra-map-geo-tests)
    endif()
    find_file(MULTRA_PROJ_DATABASE proj.db
        HINTS ${CMAKE_PREFIX_PATH} "${PROJ_DIR}" "${PROJ_DIR}/../../../.."
        PATH_SUFFIXES share/proj REQUIRED)
    add_custom_target(multra-proj-data ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,$<CONFIG>/,>data/proj"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MULTRA_PROJ_DATABASE}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,$<CONFIG>/,>data/proj/proj.db"
        DEPENDS multra-data)
    install(FILES "${MULTRA_PROJ_DATABASE}" DESTINATION bin/data/proj)
    add_subdirectory(src/shell)
endif()
