cmake_minimum_required(VERSION 3.21)

project(circle-bind VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(CMakeDependentOption)
include(cmake/CPM.cmake)

cmake_dependent_option(CIRCLE_BIND_TEST "Generate test target for circle::bind" ON "${PROJECT_IS_TOP_LEVEL}" OFF)

add_subdirectory(src)

if (CIRCLE_BIND_TEST)
    add_subdirectory(test)
endif()
