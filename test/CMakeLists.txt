CPMAddPackage("gh:catchorg/Catch2@3.5.3")
list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")

enable_testing()
include(Catch)

add_executable(circle-reactive-test
    bind_test.cpp
    observer_test.cpp
    property_test.cpp
    ptr_test.cpp
    signal_test.cpp
)

target_link_libraries(circle-reactive-test PRIVATE
    circle::reactive
    Catch2::Catch2WithMain
)

catch_discover_tests(circle-reactive-test)

if (REACTIVE_CPP_COVERAGE)
    add_gcovr_target_html(circle-reactive-coverage
        TEST_TARGET circle-reactive-test
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/include
    )

    add_gcovr_target_xml(circle-reactive-coverage-xml
        TEST_TARGET circle-reactive-test
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/include
    )
endif()