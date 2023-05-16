include("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

// https://github.com/emscripten-core/emscripten/wiki/WebAssembly-Standalone
message(WARNING "The WASI still doesn't support C++ exceptions, the binary will not be fully standalone!")

set(LINK_OPTIONS "-sSTANDALONE_WASM")
