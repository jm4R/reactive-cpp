cmake -S . --preset=wasm-debug
cmake --build build/wasm-debug --parallel 4
node ./build/wasm-debug/test/circle-reactive-test

cmake -S . --preset=wasm-release
cmake --build build/wasm-release --parallel 4
node ./build/wasm-release/test/circle-reactive-test

cmake -S . --preset=wasm-standalone-debug
cmake --build build/wasm-standalone-debug --parallel 4
node ./build/wasm-standalone-debug/test/circle-reactive-test
