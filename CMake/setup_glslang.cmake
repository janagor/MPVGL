execute_process(
    COMMAND  env PYTHONPATH=. python3 update_glslang_sources.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/glslang"
    RESULT_VARIABLE UPDATE_GLSLANG_SOURCES_RESULT
    OUTPUT_VARIABLE UPDATE_GLSLANG_SOURCES_OUTPUT
    ERROR_VARIABLE UPDATE_GLSLANG_SOURCES_ERROR
)
if(NOT UPDATE_GLSLANG_SOURCES_RESULT EQUAL 0)
    message( FATAL_ERROR
        "update_glslang_sources.py failed:\n${UPDATE_GLSLANG_SOURCES_ERROR}"
    )
else()
    message( STATUS
        "update_glslang_sources.py succeeded:\n${UPDATE_GLSLANG_SOURCES_OUTPUT}"
    )
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glslang)
