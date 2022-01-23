add_library(circle-bind INTERFACE
    ${circle-bind_SOURCE_DIR}/include/circle/bind/signal.hpp
)

target_include_directories(circle-bind INTERFACE
    ${circle-bind_SOURCE_DIR}/include
)

target_compile_features(circle-bind INTERFACE
    cxx_std_17
)

add_library(circle::bind ALIAS circle-bind)
