# Canvas style slices (docs/plans/CANVAS_STYLE.md); A is D423.
add_executable(multra-canvas-layers-tests ${PROJECT_SOURCE_DIR}/tests/canvas_layers_ui_tests.cpp)
target_link_libraries(multra-canvas-layers-tests PRIVATE multra_shell Qt6::Test multra_options)
target_precompile_headers(multra-canvas-layers-tests REUSE_FROM multra_shell)
add_test(NAME canvas-layers-ui COMMAND multra-canvas-layers-tests "${PROJECT_SOURCE_DIR}/data")
set_tests_properties(canvas-layers-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
add_dependencies(check multra-canvas-layers-tests)
