if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND
   NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(ERROR "Coverage report not supported for compiler: ${CMAKE_CXX_COMPILER_ID}")
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(ERROR "Coverage report not supported for build type: ${CMAKE_CXX_COMPILER_ID}")
else()
    add_compile_options(-O0 -w --coverage)
    add_link_options(--coverage)
    message(STATUS "Coverage build enabled")
endif()