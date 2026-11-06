# Owner-requested editor fixes of 2026-11-01 (/optimize-program): D429 onward.
add_executable(multra-message-label-tests ${PROJECT_SOURCE_DIR}/tests/message_label_ui_tests.cpp) # D429
target_link_libraries(multra-message-label-tests PRIVATE multra_shell Qt6::Test multra_options)
target_precompile_headers(multra-message-label-tests REUSE_FROM multra_shell)
add_test(NAME message-label-ui COMMAND multra-message-label-tests "${PROJECT_SOURCE_DIR}/data")
set_tests_properties(message-label-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
add_dependencies(check multra-message-label-tests)
add_executable(multra-canvas-dash-tests ${PROJECT_SOURCE_DIR}/tests/canvas_dash_ui_tests.cpp) # D430
target_link_libraries(multra-canvas-dash-tests PRIVATE multra_shell Qt6::Test multra_options)
target_precompile_headers(multra-canvas-dash-tests REUSE_FROM multra_shell)
add_test(NAME canvas-dash-ui COMMAND multra-canvas-dash-tests "${PROJECT_SOURCE_DIR}/data")
set_tests_properties(canvas-dash-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
add_dependencies(check multra-canvas-dash-tests)
add_executable(multra-canvas-dock-tests ${PROJECT_SOURCE_DIR}/tests/canvas_dock_ui_tests.cpp) # D431
target_link_libraries(multra-canvas-dock-tests PRIVATE multra_shell Qt6::Test multra_options)
target_precompile_headers(multra-canvas-dock-tests REUSE_FROM multra_shell)
add_test(NAME canvas-dock-ui COMMAND multra-canvas-dock-tests "${PROJECT_SOURCE_DIR}/data")
set_tests_properties(canvas-dock-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
add_dependencies(check multra-canvas-dock-tests)
# M10.2 G2 (TRIP_GENERATION §13, D471): zone attributes, the Trip-rate sets dialog, the Trip generation page.
add_executable(multra-trip-generation-ui-tests ${PROJECT_SOURCE_DIR}/tests/trip_generation_ui_tests.cpp)
target_link_libraries(multra-trip-generation-ui-tests PRIVATE multra_shell Qt6::Test multra_options)
target_precompile_headers(multra-trip-generation-ui-tests REUSE_FROM multra_shell)
add_test(NAME trip-generation-ui COMMAND multra-trip-generation-ui-tests "${PROJECT_SOURCE_DIR}/data" "${PROJECT_SOURCE_DIR}/tests/trip-generation.traffic.json" "${PROJECT_SOURCE_DIR}/tests/data/trip-generation/trip-rates/example-4-1.json")
set_tests_properties(trip-generation-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)
add_dependencies(check multra-trip-generation-ui-tests)
# M10.3 D2 (TRIP_DISTRIBUTION §13, D474): the Trip distribution page and its two dialogs.
add_executable(multra-trip-distribution-ui-tests ${PROJECT_SOURCE_DIR}/tests/trip_distribution_ui_tests.cpp)
target_link_libraries(multra-trip-distribution-ui-tests PRIVATE multra_shell Qt6::Test multra_options)
target_precompile_headers(multra-trip-distribution-ui-tests REUSE_FROM multra_shell)
add_test(NAME trip-distribution-ui COMMAND multra-trip-distribution-ui-tests "${PROJECT_SOURCE_DIR}/data" "${PROJECT_SOURCE_DIR}/tests/trip-generation.traffic.json" "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution/trip-rates/one-way.json" "${PROJECT_SOURCE_DIR}/tests/data/trip-distribution/deterrence/hand.json")
set_tests_properties(trip-distribution-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)
add_dependencies(check multra-trip-distribution-ui-tests)
