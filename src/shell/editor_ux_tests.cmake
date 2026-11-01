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
