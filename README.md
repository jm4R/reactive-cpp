# reactive-cpp

Branch          | Appveyor | Codecov |
:-------------: | -------- | ------- |
[`master`](https://github.com/jm4R/reactive-cpp/tree/master) | [![Build status](https://ci.appveyor.com/api/projects/status/ix6o5njakdpqvbrl/branch/master?svg=true)](https://ci.appveyor.com/project/jm4R/reactive-cpp/branch/master) | [![codecov](https://codecov.io/gh/jm4R/reactive-cpp/branch/master/graph/badge.svg)](https://codecov.io/gh/jm4R/reactive-cpp) |
[`develop`](https://github.com/jm4R/reactive-cpp/tree/develop) | [![Build status](https://ci.appveyor.com/api/projects/status/ix6o5njakdpqvbrl/branch/develop?svg=true)](https://ci.appveyor.com/project/jm4R/reactive-cpp/branch/master) | [![codecov](https://codecov.io/gh/jm4R/reactive-cpp/branch/develop/graph/badge.svg)](https://codecov.io/gh/jm4R/reactive-cpp) |

#### CI build targets
* g++-7
* clang-9
* MSVC++ 2019
* MSVC++ 2022
* emscripten 3.1


## Introduction

Property binding library lazy value evaluation.

Library is in development, but here's an already-working demo:

```cpp
using namespace circle;

property<int> a = 0;
property<int> b = 0;

property max = BIND(a, b, std::max(a, b));

a = 60;
b = 75;
std::cout << "max(" << *a  << ", " << *b << ") = " << *max << std::endl;

a = 120;
std::cout << "max(" << *a  << ", " << *b << ") = " << *max << std::endl;
```

The example prints:

> max(60, 75) = 75\
> max(120, 75) = 120