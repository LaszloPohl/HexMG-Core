file(MAKE_DIRECTORY "${FIXTURE_ROOT}")
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef run_id)
execute_process(COMMAND "${READER}" "${FIXTURE_ROOT}/${run_id}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
message("${output}${error}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "OPTIONAL reader tests failed: ${result}")
endif()
