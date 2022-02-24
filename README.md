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

Property binding library with lazy value evaluation.

Library needs a deeper documentation, but here's a demo:

### circle::signal

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

### circle::property

```cpp
using namespace circle;
property<std::string> p;
p.value_changed().connect([](const std::string& num){ std::cout << "The text is " << num << std::endl; });
p = "foo";
p = "foo";
p = "bar";
```

The example prints:

> The text is foo\
> The text is bar

### circle::binding (BIND)

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

## Public types

In header `<circle/reactive/signal.hpp>`

* `signal<Args...>`
* `connection`
* `scoped_connection`
* `connection_blocker`

In header `<circle/reactive/property.hpp>`

* `property<T>`
* `property_ref<T>`
* `value_provider` abstract class
* `value_provider_ptr` alias
* `is_property<T>` type trait

In header `<circle/reactive/properties_observer.hpp>`

* `properties_observer<Properties...>`

In header `<circle/reactive/bind.hpp>`

* `binding<T, DependentProperties...>`
* `BIND(dependent_list..., expression)` helper macro
