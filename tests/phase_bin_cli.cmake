foreach(width IN ITEMS inf nan -inf 0 -1 1e-300)
    execute_process(COMMAND "${CLI}" --project "${PROJECT}" --arrival-phases --phase-bin "${width}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 1 OR NOT error MATCHES "(finite positive|histogram limit)")
        message(FATAL_ERROR "phase-bin ${width}: result=${result}, error=${error}")
    endif()
endforeach()
foreach(width IN ITEMS 10 1e20)
    execute_process(COMMAND "${CLI}" --project "${PROJECT}" --arrival-phases --phase-bin "${width}"
                    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR output STREQUAL "")
        message(FATAL_ERROR "phase-bin ${width}: result=${result}, error=${error}")
    endif()
endforeach()
