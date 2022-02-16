function(add_gcovr_target_html _target)
    find_program(GCOVR_EXECUTABLE gcovr)
    mark_as_advanced(GCOVR_EXECUTABLE)
    if(NOT GCOVR_EXECUTABLE)
        message(FATAL_ERROR "gcovr not found!")
    endif()

    set(options "")
    set(one_value_keywords TEST_TARGET SOURCE_DIR)
    set(multi_value_args "")
    cmake_parse_arguments(arg "${options}" "${one_value_keywords}" "${multi_value_args}" ${ARGN})

    if(NOT arg_TEST_TARGET)
        message(FATAL_ERROR "No TEST_TARGET specified in add_gcovr_target_html(${_target} ...)")
    endif()
    if(NOT arg_SOURCE_DIR)
        message(FATAL_ERROR "No SOURCE_DIR specified in add_gcovr_target_html(${_target} ...)")
    endif()

    set (gcovr_command ${GCOVR_EXECUTABLE}
        --root=${PROJECT_SOURCE_DIR}
        --filter=${arg_SOURCE_DIR}
        --html
        --html-details
        -o "$<TARGET_FILE_DIR:${arg_TEST_TARGET}>/$<TARGET_FILE_NAME:${arg_TEST_TARGET}>-coverage.html"
        $<TARGET_FILE_DIR:${arg_TEST_TARGET}>
    )

    add_custom_target(${_target}
        COMMAND $<TARGET_FILE:${arg_TEST_TARGET}>
        COMMAND ${gcovr_command}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${arg_TEST_TARGET}>
        COMMENT "Generate gcovr HTML report for ${arg_TEST_TARGET}"
    )
endfunction()

function(add_gcovr_target_xml _target)
    find_program(GCOVR_EXECUTABLE gcovr)
    mark_as_advanced(GCOVR_EXECUTABLE)
    if(NOT GCOVR_EXECUTABLE)
        message(FATAL_ERROR "gcovr not found!")
    endif()

    set(options "")
    set(one_value_keywords TEST_TARGET SOURCE_DIR)
    set(multi_value_args "")
    cmake_parse_arguments(arg "${options}" "${one_value_keywords}" "${multi_value_args}" ${ARGN})

    if(NOT arg_TEST_TARGET)
        message(FATAL_ERROR "No TEST_TARGET specified in add_gcovr_target_html(${_target} ...)")
    endif()
    if(NOT arg_SOURCE_DIR)
        message(FATAL_ERROR "No SOURCE_DIR specified in add_gcovr_target_html(${_target} ...)")
    endif()

    set (gcovr_command ${GCOVR_EXECUTABLE}
        --root=${PROJECT_SOURCE_DIR}
        --filter=${arg_SOURCE_DIR}
        --xml
        -o "$<TARGET_FILE_DIR:${arg_TEST_TARGET}>/$<TARGET_FILE_NAME:${arg_TEST_TARGET}>-coverage.xml"
        $<TARGET_FILE_DIR:${arg_TEST_TARGET}>
    )

    add_custom_target(${_target}
        COMMAND $<TARGET_FILE:${arg_TEST_TARGET}>
        COMMAND ${gcovr_command}
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${arg_TEST_TARGET}>
        COMMENT "Generate gcovr XML report for ${arg_TEST_TARGET}"
    )
endfunction()

if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
   NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(FATAL_ERROR "Coverage report not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "Coverage report not supported for build type: ${CMAKE_CXX_COMPILER_ID}")
else()
    add_compile_options(-O0 -w -fprofile-arcs -ftest-coverage --coverage)
    add_link_options(-lgcov --coverage)
    message(STATUS "Coverage build enabled")
endif()