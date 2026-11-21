# `check` runs every registered test, so it must build every executable first: a test binary left
# out ran stale against rebuilt shared libraries and crashed after a struct grew (D613's session:
# behaviour-lab-ui, cli-consistency). Collected after every add_subdirectory, so new targets join.
function(multra_check_depends_on_all_executables dir)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        if(type STREQUAL "EXECUTABLE")
            add_dependencies(check ${target})
        endif()
    endforeach()
    get_property(children DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        multra_check_depends_on_all_executables("${child}")
    endforeach()
endfunction()
multra_check_depends_on_all_executables("${PROJECT_SOURCE_DIR}")
