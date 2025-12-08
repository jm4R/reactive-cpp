# reactive-cpp

**A powerful yet lightweight C++ library for reactive properties, bindings, and signals.**

A minimal alternative to [RxCpp](https://reactivex.io/RxCpp/) and [LiveCells](https://gutev.dev/live_cells_cpp/) — ideal for MVVM, game logic, tools, and UI frameworks.

Branch          | Appveyor | Codecov |
:-------------: | -------- | ------- |
[`master`](https://github.com/jm4R/reactive-cpp/tree/master) | [![Build status](https://ci.appveyor.com/api/projects/status/ix6o5njakdpqvbrl/branch/master?svg=true)](https://ci.appveyor.com/project/jm4R/reactive-cpp/branch/master) | [![codecov](https://codecov.io/gh/jm4R/reactive-cpp/branch/master/graph/badge.svg)](https://codecov.io/gh/jm4R/reactive-cpp) |
[`develop`](https://github.com/jm4R/reactive-cpp/tree/develop) | [![Build status](https://ci.appveyor.com/api/projects/status/ix6o5njakdpqvbrl/branch/develop?svg=true)](https://ci.appveyor.com/project/jm4R/reactive-cpp/branch/develop) | [![codecov](https://codecov.io/gh/jm4R/reactive-cpp/branch/develop/graph/badge.svg)](https://codecov.io/gh/jm4R/reactive-cpp) |

## Overview

Modern C++ still lacks a simple way to express:

- reactive **properties** (`property<T>`)
- automatic **bindings** between them (`c = a + b`)
- lightweight **observation** of changes (connections)
- value-based **reactive state**
- automatic binding invalidation

`reactive-cpp` provides exactly this with an API inspired by Qt/QML and functional reactive patterns — but with pure C++ code, without heavy abstractions or generators.

#### Key features:

- ⚡ **Instant propagation** of dependent values
- 🧩 **Simple and declarative** usage
- 📦 **Depandency-free**, STL only
- 📐 **Perfect for MVVM** (ViewModel properties)
- 🚀 Suitable for game engines, editors, tools, and real-time apps
- 💡 [Glitch-free](https://en.wikipedia.org/wiki/Reactive_programming#Glitches) updates

## Core Concepts

#### property

```cpp
using namespace circle;
property<std::string> p;
p.value_changed().connect(
    [](const std::string& num)
    {
        std::cout << "The text is " << num << std::endl;
    }
);
p = "foo";
p = "foo";
p = "bar";
```

The example prints:

> The text is foo\
> The text is bar

#### binding

```cpp
using namespace circle;

property<int> a = 0;
property<int> b = 0;

property max = BIND(a, b, std::max(a, b)); // simple version

a = 60;
b = 75;
std::cout << "max(" << *a  << ", " << *b << ") = " << *max << std::endl;

a = 120;
std::cout << "max(" << *a  << ", " << *b << ") = " << *max << std::endl;

struct
{
    property<int> val;
} nested;

property max = BIND(a, (nested.val, nested_val), std::max(a, nested_val)); // or name parameter explicitly
```

The example prints:

> max(60, 75) = 75\
> max(120, 75) = 120


#### signal

```cpp
using namespace circle;
signal<int> s;
s.connect([](int num){ std::cout << "The num is " << num; });
s.connect([](int num){ std::cout << ", and now the num is still " << num << std::endl; });
s.emit(2);
s.emit(-5);
```

The example prints:

> The num is 2, and now the num is still 2\
> The num is -5, and now the num is still -5

## Installation

This is a cmake-based project. You can choose simple local integration:

```cmake
add_subdirectory(reactive-cpp)
target_link_libraries(myapp PRIVATE circle::reactive)
```

You can also add `reactive-cpp` via CMake/FetchContent or [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake):

```cmake
CPMAddPackage("gh:jm4R/reactive-cpp#master")
target_link_libraries(myapp PRIVATE circle::reactive)
```

## CI build targets
* GCC 10
* Clang 12
* MSVC 2019
* MSVC 2022
* Emscripten 3.1

## Comparison With Similar Libraries

`reactive-cpp` was created because existing C++ reactive libraries solve different problems or introduce limitations that are undesirable in state-based reactive architectures such as MVVM.

### KDBindings
It began development at roughly the same time as `reactive-cpp`; both projects were motivated by similar gaps in C++ for expressing reactive state and MVVM-style property bindings.
KDBindings provides property bindings and signals, but at the time `reactive-cpp` was conceived it was **not glitch-free**, meaning that dependent properties could temporarily observe inconsistent intermediate states during update cascades.  
For applications where correctness and consistency between related properties matters (for example in UI view-models), this behavior can be problematic.

`reactive-cpp` guarantees **glitch-free updates** by construction: all dependencies are recalculated in a stable order before any observers are notified.


### LiveCells
LiveCells is conceptually the closest library to `reactive-cpp`, and in many use cases the two could be used interchangeably.  
Both libraries provide value-based reactivity, automatic propagation, and a declarative model of dependent values.

However, the internal binding mechanisms differ significantly:

- **LiveCells automatically detects dependencies** by evaluating the lambda and tracking every `cell` access.  
  This allows very compact syntax, but also permits **dangerous constructions**, such as lambdas that capture objects by reference.  
  If the captured object goes out of scope, this may lead to **dangling references** inside the reactive graph.

- **reactive-cpp intentionally avoids automatic dependency discovery.**  
  Instead, each binding **explicitly lists the properties and objects it depends on**, making the dependency graph fully visible and predictable.  
  reactive-cpp also includes **safety mechanisms that automatically invalidate a binding when one of its dependencies is destroyed**, preventing use-after-free scenarios.

### RxCpp
RxCpp implements a full ReactiveX-style **stream-based FRP** system, suited for asynchronous flows, event pipelines, schedulers, and time-based operators.  
It is extremely capable, but operates in a different conceptual domain:  
**event streams**, not **reactive state**.

`reactive-cpp` focuses exclusively on **value reactivity**:

- stable and deterministic state  
- glitch-free propagation  
- straightforward binding relationships  
- ideal for MVVM, game logic, tools, and UI models

If you need declarative, always-consistent reactive properties rather than observable event streams, `reactive-cpp` offers a lightweight and direct solution.

## Is this library production-ready?

Although the library is still young, it is already proving itself in some proprietary commercial products.
Feel free to report any bugs, suggestions & feedback.
