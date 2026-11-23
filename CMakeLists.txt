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
# D488: the desktop preset links the multra libraries as shared objects on Linux. Static, each of
# some 85 test executables carried its own copy of libmultra_shell's debug info (30 GB per Debug
# tree) and one shell edit relinked 118 of them. Releases, packages and Windows stay static.
option(MULTRA_SHARED_LIBS "Build the multra libraries as shared objects (not on Windows)" OFF)
set(MULTRA_LIBRARY_TYPE STATIC)
if(MULTRA_SHARED_LIBS AND NOT WIN32)
    set(MULTRA_LIBRARY_TYPE SHARED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_LINK_DEPENDS_NO_SHARED ON) # a changed .so does not relink what uses it
endif()
include(CTest)
add_library(multra_options INTERFACE)
target_compile_definitions(multra_options INTERFACE
    MULTRA_VERSION="${PROJECT_VERSION}"
    MULTRA_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
if(MSVC)
    target_compile_options(multra_options INTERFACE /W4 /permissive- /utf-8 /fp:strict)
else()
    target_compile_options(multra_options INTERFACE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off
        # Aggregates keep trailing members at their defaults on purpose (that warning was 1120 of 1151
        # lines); GCC 13's -Wdangling-reference flags every lookup that returns into its container (D599).
        -Wno-missing-field-initializers $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,14>>:-Wno-dangling-reference>)
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
include(cmake/debug_optimisation.cmake) # D621: -Og for GCC/Clang Debug builds

# System/vcpkg packages; no network downloads happen implicitly during configure.
find_package(nlohmann_json 3.11 CONFIG QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_path(MULTRA_JSON_INCLUDE_DIR nlohmann/json.hpp REQUIRED)
    add_library(multra_json INTERFACE)
    target_include_directories(multra_json SYSTEM INTERFACE "${MULTRA_JSON_INCLUDE_DIR}")
    add_library(nlohmann_json::nlohmann_json ALIAS multra_json)
endif()

# The four engines import nothing and nothing of each other (check_architecture); one target each,
# so a stray include between them fails at link time as well.
add_library(multra_macro ${MULTRA_LIBRARY_TYPE} src/macro/graph.cpp src/macro/assign.cpp src/macro/trip_generation.cpp
    src/macro/trip_distribution.cpp src/macro/mode_choice.cpp)
add_library(multra_meso ${MULTRA_LIBRARY_TYPE} src/meso/graph.cpp src/meso/demand.cpp src/meso/node_model.cpp
    src/meso/link_model.cpp src/meso/loading.cpp)
add_library(multra_analysis ${MULTRA_LIBRARY_TYPE} src/analysis/signalised.cpp src/analysis/lane_flow.cpp
    src/analysis/critical.cpp src/analysis/queue.cpp src/analysis/qap.cpp src/analysis/permitted.cpp)
foreach(engine multra_macro multra_meso multra_analysis)
    target_link_libraries(${engine} PRIVATE multra_options)
endforeach()
add_library(multra_core ${MULTRA_LIBRARY_TYPE}
    src/core/demand.cpp src/core/following.cpp src/core/routes.cpp src/core/speed_decision.cpp src/core/speed_area.cpp
    src/core/signals.cpp src/core/red_running.cpp src/core/start_up.cpp src/core/diffusion.cpp src/core/yield_time.cpp src/core/caps.cpp src/core/join_anticipation.cpp src/core/simulation.cpp src/core/insertion.cpp src/core/publish.cpp src/core/validate.cpp src/core/conflicts.cpp
    src/core/lanes.cpp src/core/routing.cpp src/core/w74.cpp src/core/perception.cpp src/core/lateral.cpp src/core/lateral_motion.cpp src/core/lane_swap.cpp src/core/courtesy.cpp src/core/leaders.cpp src/core/speed_control.cpp src/core/parking.cpp src/core/retune.cpp src/core/missed_exit.cpp)
target_link_libraries(multra_core PRIVATE multra_options)

add_library(multra_model ${MULTRA_LIBRARY_TYPE} src/model/network/junction_templates.cpp src/model/network/geometry.cpp
    src/model/network/connector_equation.cpp src/model/network/compile.cpp src/model/network/validate.cpp src/model/network/connector_geometry.cpp
    src/model/network/diagnostics.cpp src/model/network/connector_paths.cpp src/model/network/road_boundaries.cpp src/model/network/connector_surface.cpp
    src/model/network/sections.cpp src/model/network/markings.cpp src/model/network/rotation.cpp src/model/network/head_slots.cpp src/model/demand/signal_control.cpp src/model/demand/macro_compile.cpp src/model/demand/meso_compile.cpp src/model/demand/coarse_compile.cpp src/model/demand/node_analysis_compile.cpp src/model/demand/seam_checks.cpp src/model/demand/meso_paths.cpp src/model/demand/meso_street_segment_paths.cpp
    src/model/network/routing.cpp src/model/network/lane_family.cpp src/model/network/routeless.cpp src/model/network/right_of_way.cpp src/model/network/merge_groups.cpp src/model/network/priority_markers.cpp src/model/network/travel_time.cpp src/model/network/polygon.cpp src/model/network/nodes.cpp src/model/network/street_segments.cpp src/model/network/street_segment_crossings.cpp src/model/network/coarse_nodes.cpp src/model/network/node_detail.cpp src/model/network/study_area.cpp src/model/network/node_membership.cpp src/model/network/node_suggestions.cpp
    src/model/network/conflict_surface.cpp src/model/network/conflict_groups.cpp src/model/network/conflict_coverage.cpp src/model/network/conflict_crossings.cpp src/model/network/conflict_polygons.cpp
    src/model/network/conflict_polygon_math.cpp
    src/model/network/conflict_bands.cpp
    src/model/network/conflict_mouth_bands.cpp src/model/network/automatic_conflicts.cpp src/model/network/branching_compile.cpp)
target_link_libraries(multra_model PUBLIC multra_core multra_macro multra_meso multra_analysis PRIVATE multra_options)
target_sources(multra_model PRIVATE src/model/network/approach_layout.cpp src/model/demand/nema_timing.cpp src/model/demand/nema_check.cpp)
# Run-view geometry is Qt-free and independently testable; the core never imports it.
add_library(multra_vehicle_pose ${MULTRA_LIBRARY_TYPE} src/editor/vehicle_pose.cpp src/editor/rear_axle_pose.cpp src/editor/lane_change_pose.cpp)
target_link_libraries(multra_vehicle_pose PUBLIC multra_model PRIVATE multra_options)
add_library(multra_eval ${MULTRA_LIBRARY_TYPE} src/eval/discharge.cpp src/eval/discharge_passage.cpp src/eval/summary.cpp src/eval/movement.cpp src/eval/nodes.cpp src/eval/sections.cpp src/eval/los.cpp src/eval/lane_changes.cpp src/eval/segment_times.cpp src/eval/stop_lines.cpp src/eval/priority_lines.cpp src/eval/fctl_reference.cpp src/eval/arrival_phases.cpp src/eval/dead_end_waits.cpp src/eval/lots.cpp)
target_link_libraries(multra_eval PUBLIC multra_core PRIVATE multra_options)
# M5.2 batches: independent seeds and their aggregate. Core and eval only (check_architecture).
add_library(multra_runner ${MULTRA_LIBRARY_TYPE} src/runner/batch.cpp src/runner/compare.cpp)
target_link_libraries(multra_runner PUBLIC multra_eval PRIVATE multra_options)
add_library(multra_project ${MULTRA_LIBRARY_TYPE} src/project/load.cpp src/project/parse.cpp src/project/output.cpp
    src/project/document.cpp src/project/signal_migration.cpp src/project/definition.cpp src/project/run.cpp src/project/display.cpp src/project/diagnostics.cpp
    src/project/junction_template_codec.cpp src/project/junction_recipe_codec.cpp src/project/node_codec.cpp src/project/study_period_codec.cpp src/project/study_area_codec.cpp src/project/study_area_run.cpp src/project/study_area_macro.cpp src/project/study_area_meso.cpp src/project/side_files.cpp src/project/export_marking.cpp src/project/mode_codec.cpp src/project/link_type_codec.cpp src/project/street_segment_codec.cpp src/project/link_speed_codec.cpp src/project/macro_input.cpp src/project/macro_settings.cpp src/project/meso_settings.cpp src/project/meso_input.cpp src/project/meso_run.cpp src/project/readiness.cpp src/project/node_proposals.cpp src/project/meso_output.cpp src/project/macro_run.cpp src/project/macro_output.cpp src/project/input_sha256.cpp src/project/input_manifest.cpp src/project/behaviour_library.cpp src/project/priority_rule_classes.cpp src/project/discharge_json.cpp src/project/evaluation.cpp src/project/demand_paths.cpp src/project/demand_preview.cpp src/project/demand_catalog.cpp src/project/demand_time_types.cpp src/project/station_routing.cpp src/project/batch_output.cpp src/project/node_output.cpp src/project/comparison_output.cpp src/project/evaluation_period.cpp src/project/clamp_rules.cpp src/project/random_streams_codec.cpp src/project/los_output.cpp src/project/counted_volumes.cpp src/project/map_codec.cpp src/project/route_destination.cpp src/project/speed_decisions.cpp src/project/speed_areas.cpp src/project/speed_curve.cpp src/project/lane_preference.cpp src/project/zone_codec.cpp src/project/od_demand.cpp src/project/parking_compile.cpp
    src/project/parallel_seeds.cpp)
# D152: the seed scheduler's worker threads live here; runner stays thread-free (check_architecture).
find_package(Threads REQUIRED)
target_sources(multra_project PRIVATE src/project/cost_matrix_codec.cpp src/project/deterrence_codec.cpp src/project/trip_distribution_run.cpp src/project/zone_skim.cpp src/project/procedure_feedback.cpp src/project/dynamic_assignment.cpp src/project/dynamic_assignment_run.cpp src/project/route_costs.cpp src/project/dynamic_assignment_loop.cpp src/project/meso_assignment.cpp src/project/mode_choice_codec.cpp src/project/period_factor_codec.cpp src/project/mode_choice_run.cpp src/project/mode_choice_output.cpp src/project/procedure_codec.cpp src/project/procedure_run.cpp src/project/procedure_output.cpp src/project/trip_distribution_output.cpp src/project/trip_rate_codec.cpp src/project/trip_generation_run.cpp src/project/trip_generation_output.cpp src/project/macro_run_definition.cpp src/project/observed_schema.cpp src/project/document_envelope.cpp src/project/network_fields.cpp src/project/network_link_codec.cpp src/project/network_connector_codec.cpp src/project/network_signal_head_codec.cpp src/project/signal_control_codec.cpp src/project/right_of_way_codec.cpp src/project/measurement_codec.cpp src/project/demand_codec.cpp src/project/routing_codec.cpp src/project/object_csv.cpp src/project/network_schema.cpp src/project/subarea_consistency.cpp)
target_sources(multra_project PRIVATE src/project/meso_street_segment_demand.cpp src/project/node_analysis_run.cpp src/project/node_analysis_permitted.cpp src/project/node_analysis_output.cpp src/project/node_analysis_report.cpp src/project/build_metadata.cpp)
target_sources(multra_project PRIVATE src/project/node_model_demand.cpp)
target_sources(multra_project PRIVATE src/project/node_analysis_fill.cpp)
target_sources(multra_project PRIVATE src/project/node_analysis_source_codec.cpp)
target_sources(multra_project PRIVATE src/project/node_analysis_comparison.cpp)
target_sources(multra_project PRIVATE src/project/utdf_legs.cpp src/project/utdf_signals.cpp src/project/utdf_export.cpp)
target_link_libraries(multra_project PUBLIC multra_model multra_eval multra_runner Threads::Threads
    nlohmann_json::nlohmann_json PRIVATE multra_options)

# nlohmann/json is a large header-only template library: 1.47 s to parse and instantiate here,
# against 0.36 s for <nlohmann/json_fwd.hpp>. src/project/json.hpp includes the forward
# declaration, so only a translation unit that builds or reads a Json pays for the definition,
# and precompiling it on the multi-source targets turns that into one cost per target. A
# single-source target gets no PCH of its own -- it would cost about what it saves -- but it can
# REUSE_FROM one that already exists, which src/shell does for the UI test executables.
target_precompile_headers(multra_project PRIVATE <nlohmann/json.hpp>)

target_sources(multra_model PRIVATE src/model/demand/junction_seams.cpp src/model/demand/nema_site.cpp)
add_library(multra_commands ${MULTRA_LIBRARY_TYPE} src/commands/history.cpp src/commands/network_commands.cpp
    src/commands/junction_fill.cpp src/commands/junction_fill_polygon.cpp src/commands/transition_fill.cpp src/commands/junction_edit_commands.cpp
    src/commands/junction_fill_restore.cpp src/commands/junction_fill_geometry.cpp src/commands/junction_fill_t_cuts.cpp src/commands/junction_fill_references.cpp src/commands/junction_fill_signals.cpp src/commands/node_detail_priority.cpp
    src/commands/split_link.cpp src/commands/connector_commands.cpp src/commands/delete_objects.cpp
    src/commands/demand_commands.cpp src/commands/catalog_commands.cpp src/commands/street_segment_commands.cpp src/commands/behaviour_commands.cpp src/commands/appearance_commands.cpp src/commands/link_geometry_commands.cpp
    src/commands/right_of_way_commands.cpp src/commands/priority_marker_commands.cpp src/commands/conflict_authoring.cpp
    src/commands/junction_commands.cpp src/commands/node_commands.cpp src/commands/coarse_commands.cpp src/commands/fill_commands.cpp src/commands/node_detail_commands.cpp src/commands/conflict_follow.cpp src/commands/conflict_group_commands.cpp src/commands/map_commands.cpp src/commands/zone_commands.cpp)
target_link_libraries(multra_commands PUBLIC multra_project PRIVATE multra_options)
target_sources(multra_commands PRIVATE src/commands/trip_rate_commands.cpp src/commands/mode_choice_commands.cpp src/commands/procedure_commands.cpp src/commands/macro_run_commands.cpp src/commands/node_analysis_commands.cpp)
target_precompile_headers(multra_commands PRIVATE <nlohmann/json.hpp>)
# D165 behaviour lab: the ring, its run and live retune. Qt-free; the shell draws it (check_architecture).
add_library(multra_lab ${MULTRA_LIBRARY_TYPE} src/lab/lab_settings.cpp src/lab/lab_network.cpp src/lab/lab_oval.cpp src/lab/lab_urban.cpp src/lab/lab_run.cpp)
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
target_include_directories(multra_project PRIVATE ${CMAKE_BINARY_DIR}/generated)
add_dependencies(multra_project multra-build-info)
include(cmake/tools.cmake) # D630: the CLI, benchmarks, fixtures and sweeps
install(DIRECTORY data/ DESTINATION bin/data)

include(cmake/check_target.cmake) # D630: the check tools and the `check` target

if(BUILD_TESTING)
    include(cmake/model_tests.cmake) # D630: multra-tests and its groups
    include(cmake/cli_tests.cmake) # D630: tool, CLI and script tests
    add_dependencies(check multra-tests)
endif()

if(MULTRA_BUILD_DESKTOP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network)
    find_package(PROJ CONFIG REQUIRED)
    add_library(multra_geo ${MULTRA_LIBRARY_TYPE} src/geo/projection.cpp src/geo/tile_grid.cpp src/geo/projection_data.cpp)
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
include(cmake/check_builds_all.cmake)
