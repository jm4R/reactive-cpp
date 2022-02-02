add_library(circle-bind INTERFACE
    ${circle-bind_SOURCE_DIR}/include/circle/bind/bind.hpp
    ${circle-bind_SOURCE_DIR}/include/circle/bind/properties_observer.hpp
    ${circle-bind_SOURCE_DIR}/include/circle/bind/property.hpp
    ${circle-bind_SOURCE_DIR}/include/circle/bind/signal.hpp
    ${circle-bind_SOURCE_DIR}/include/circle/bind/utils.hpp
)

target_include_directories(circle-bind INTERFACE
    ${circle-bind_SOURCE_DIR}/include
)

target_compile_features(circle-bind INTERFACE
    cxx_std_17
)

add_library(circle::bind ALIAS circle-bind)
