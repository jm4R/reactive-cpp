CPMAddPackage("gh:catchorg/Catch2@2.13.8")
list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/contrib")

enable_testing()
include(Catch)

add_executable(circle-reactive-test
    bind_test.cpp
    properties_observer_test.cpp
    property_test.cpp
    signal_test.cpp

    main_test.cpp
)

target_link_libraries(circle-reactive-test PRIVATE
    circle::reactive
    Catch2::Catch2
)

catch_discover_tests(circle-reactive-test)