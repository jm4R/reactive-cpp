add_library(circle-reactive INTERFACE
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/bind.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/observer.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/property.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/ptr.hpp
    ${reactive-cpp_SOURCE_DIR}/include/circle/reactive/signal.hpp
)

target_include_directories(circle-reactive INTERFACE
    ${reactive-cpp_SOURCE_DIR}/include
)

target_compile_features(circle-reactive INTERFACE
    cxx_std_20
)

add_library(circle::reactive ALIAS circle-reactive)
