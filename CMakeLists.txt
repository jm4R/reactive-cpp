cmake_minimum_required(VERSION 3.21)

project(reactive-cpp VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(CMakeDependentOption)
include(cmake/CPM.cmake)

cmake_dependent_option(REACTIVE_CPP_TEST "Generate test target for circle::reactive" ON "${PROJECT_IS_TOP_LEVEL}" OFF)

add_subdirectory(src)

if (REACTIVE_CPP_TEST)
    add_subdirectory(test)
endif()
