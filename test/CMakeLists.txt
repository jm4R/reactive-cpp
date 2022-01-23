CPMAddPackage("gh:catchorg/Catch2@2.13.8")

add_executable(circle-bind-test
    signal_test.cpp
    main_test.cpp
)

target_link_libraries(circle-bind-test PRIVATE
    circle::bind
    Catch2::Catch2
)

