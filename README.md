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


### Detailed types

When creating a binding, we often encounter the requirement that some object must be alive (eg. not dangling) to be sure that binding is valid. The `std::weak_ptr` could be used in some cases, but it will never guarantee that the binding is fully invalidated, we can only use its state inside a binding logic. It might be more convenient to use following utilities:

#### circle::enable_tracking_ptr and circle::tracking_ptr

This utility allows tracking object (of class derived from `enable_tracking_ptr`) lifetime. It informs the connected `tracking_ptr` about origin object destruction and address changes (through move operation). The `enable_tracking_ptr` class must implement destructor, move constructor and assignment operator (or `=delete` it). Here's the example:

```cpp
using namespace circle;

struct tracked_test : public enable_tracking_ptr<tracked_test>
{
    using base = enable_tracking_ptr<tracked_test>;
    tracked_test() = default;
    ~tracked_test() { this->call_before_destroyed(); }

    tracked_test(tracked_test&& other) : base{std::move(other)}
    {
        // all operations comes before
        this->call_moved();
    }

    tracked_test& operator=(tracked_test&& other)
    {
        base::operator=(std::move(other));
        // all operations comes before
        this->call_moved();
        return *this;
    }
};

using test_ptr = tracking_ptr<tracked_test>;

int main()
{
    tracked_test obj1;
    test_ptr p = &obj1;
    assert(p.get() == &obj1);
    {
        tracked_test obj2 = std::move(obj1);
        assert(p.get() == &obj2);
    }
    assert(p.is_dangling());
    // or
    assert(p == nullptr);
    // or
    assert(!p);
}
```

The `property<T>` and `property_ptr<T>` can be treated like `enable_tracking_ptr<property>` and `tracking_ptr<property<T>>` in most contexts: it has the same behavior but additional `value_changed` signal that triggers binding update and is dereferenced directly to `const T&` (instead of `const property<T>&`) by binding logic.

**Note about tracked class inheritance:**

Let's consider `class tracked : public enable_tracking_ptr<tracked> { /*...*/ };` class. We could want to have another level of derived class, eg. `class derived : public tracked`. For given class:

* The derived class will work with `tracking_ptr<tracked>` out of the box.
* If you want to use also `tracking_ptr<derived>`, you would have to reimplement all the necessary boilerplate (destructor, move constructor and move assignment operator, derive from `enable_tracking_ptr`).

#### circle::observer
The `observer<N>` utility is a kind of generalization of `tracking_ptr` for multiple (N) trackable objects. It also detects if underlying pointers has `value_changed` special signal and reports it when necessary. It is used internally by `binding` objects but can be used as a separate utility. Here's the example:

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
