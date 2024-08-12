# reactive-cpp

Branch          | Appveyor | Codecov |
:-------------: | -------- | ------- |
[`master`](https://github.com/jm4R/reactive-cpp/tree/master) | [![Build status](https://ci.appveyor.com/api/projects/status/ix6o5njakdpqvbrl/branch/master?svg=true)](https://ci.appveyor.com/project/jm4R/reactive-cpp/branch/master) | [![codecov](https://codecov.io/gh/jm4R/reactive-cpp/branch/master/graph/badge.svg)](https://codecov.io/gh/jm4R/reactive-cpp) |
[`develop`](https://github.com/jm4R/reactive-cpp/tree/develop) | [![Build status](https://ci.appveyor.com/api/projects/status/ix6o5njakdpqvbrl/branch/develop?svg=true)](https://ci.appveyor.com/project/jm4R/reactive-cpp/branch/develop) | [![codecov](https://codecov.io/gh/jm4R/reactive-cpp/branch/develop/graph/badge.svg)](https://codecov.io/gh/jm4R/reactive-cpp) |

#### CI build targets
* g++-7
* clang-9
* MSVC++ 2019
* MSVC++ 2022
* emscripten 3.1


## Introduction

C++ property binding library.

Library needs a deeper documentation, but here's a demo:

### Basic types

#### circle::signal

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

#### circle::property

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

#### circle::binding (BIND)

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


### Detailed types

When creating a binding, we often encounter the requirement that some object must be alive (eg. not dangling) to be sure that binding is valid. The `std::weak_ptr` could be used in some cases, but it will never guarantee that the binding is fully invalidated, we can only use its state inside a binding logic. It might be more convenient to use following utilities:

#### circle::ptr

It reasembles `std::unique_ptr` logic but has additional signal that informs that the object is about to be destroyed:
```
using namespace circle;
{
    ptr<int> pint = make_ptr<int>();
    pint.before_destroyed().connect([](int val) {
        std::cout << "Value before destroyed: " << val << std::endl;
    });
    *pint = 5;
}
```
The example prints:

> Value before destroyed: 5

#### circle::tracking_ptr
The `tracking_ptr<T>` can be constructed from `ptr<T>`: it is weak, non-owning ptr that tracks the liftime of origin. It can be tested for underlying object being alive, get underlying value and it also provides `before_destroyed` signal:
```
using namespace circle;
{
    ptr<int> pint = make_ptr<int>();
    {
        auto tracking = tracking_ptr{pint};
        *pint = 10;
        assert(*tracking == 10);
        tracking.before_destroyed().connect([](int val) {
            std::cout << "Value before destroyed: " << val << std::endl;
        });
    }
    *pint = 5;
}
```
The example prints:

> Value before destroyed: 5

#### Using bindings with ptr/tracking_ptr
Just like properties, the pointers can also be "captured" by binding expressions. It will not invoke recalculations on pointee change (because no `value_changed` signal is exposed), but it invalidates the binding before the pointee is destroyed:


#### circle::observer
The `observer<N>` utility is a kind of container of `N` trackable objects (namely `property_ref`s and `tracking_ptr`s). It reports if any of tracked objects is about to be destroyed. It also detects if underlying pointers has `value_changed` special signal and reports it when necessary. It is used internally by `binding` objects but can be used as a separate utility. Here's the example:

```cpp
    property<int> a = 1;
    property<long> b = 1;
    observer obs{&a, &b};
    long c = a * b;

    // value callback:
    obs.set_callback(
        [&c, a = property_ptr{&a}, b = property_ptr{&b}] { c = *a * *b; });
    assert(c == 1);
    a = 2;
    assert(c == 2);

    // destroyed callback:
    bool destroyed{};
    obs.set_destroyed_callback([&]{ destroyed = true; });
    assert(!destroyed);

    {
        auto b3 = std::move(b2);
    }

    assert(destroyed);
```

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

In header `<circle/reactive/observer.hpp>`

* `observer<N>`

In header `<circle/reactive/bind.hpp>`

* `binding<T, DependentPropertiesOrTrackables...>`
* `BIND(dependent_list..., expression)` helper macro

In header `<circle/reactive/tracking_ptr.hpp>`

* `enable_tracking_ptr<tracked_test>` CRTP class
* `tracking_ptr<Tracked>`
