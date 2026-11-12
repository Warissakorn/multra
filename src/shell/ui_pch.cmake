# The UI executables reuse a Qt PCH. In a shared build multra_shell's own PCH carries its private
# multra_shell_EXPORTS define, which GCC refuses for any other target ("not used because
# multra_shell_EXPORTS not defined"): every UI test then parsed Qt alone. There they reuse one built
# with the same headers and multra_shell's usage requirements only -- measured 12.0 -> 10.8 s per
# test object. Static builds (Windows, release, headless) keep reusing multra_shell's PCH.
set(MULTRA_UI_PCH multra_shell)
if(MULTRA_LIBRARY_TYPE STREQUAL "SHARED")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ui_pch_anchor.cpp "// The PCH needs one source.\n")
    add_library(multra_ui_pch OBJECT ${CMAKE_CURRENT_BINARY_DIR}/ui_pch_anchor.cpp)
    target_link_libraries(multra_ui_pch PRIVATE multra_shell multra_options)
    target_precompile_headers(multra_ui_pch PRIVATE <QGraphicsView> <QPainterPath> <QWidget>)
    set(MULTRA_UI_PCH multra_ui_pch)
endif()
