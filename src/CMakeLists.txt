add_library(circle-reactive INTERFACE
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/bind.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/properties_observer.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/property.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/signal.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/tracking_ptr.hpp
)

target_include_directories(circle-reactive INTERFACE
    ${reactive-cpp_SOURCE_DIR}/include
)

target_compile_features(circle-reactive INTERFACE
    cxx_std_17
)

add_library(circle::reactive ALIAS circle-reactive)
