# UI_GEOMETRY (D680): the checks' forcing rows (U1) and the sweep of every action's dialogs (U2).
add_executable(multra-ui-geometry-tests ../../tests/ui_geometry_tests.cpp) # UI_GEOMETRY U1 (D680)
target_link_libraries(multra-ui-geometry-tests PRIVATE multra_shell multra_options)
add_test(NAME ui-geometry COMMAND multra-ui-geometry-tests)
set_tests_properties(ui-geometry PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
add_dependencies(check multra-ui-geometry-tests)
add_executable(multra-ui-dialog-sweep-tests ../../tests/ui_dialog_sweep_tests.cpp) # UI_GEOMETRY U2 (D680)
target_link_libraries(multra-ui-dialog-sweep-tests PRIVATE multra_shell Qt6::Test multra_options)
foreach(scale IN ITEMS 1 1.5 2)
    add_test(NAME ui-dialog-sweep-${scale}x COMMAND multra-ui-dialog-sweep-tests "${PROJECT_SOURCE_DIR}/data"
             "${PROJECT_SOURCE_DIR}/data/projects/four-leg-signalised.traffic.json" "${PROJECT_SOURCE_DIR}/data/projects/macro-four-step.traffic.json")
    set_tests_properties(ui-dialog-sweep-${scale}x PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=${scale}" TIMEOUT 300)
endforeach()
add_dependencies(check multra-ui-dialog-sweep-tests)
