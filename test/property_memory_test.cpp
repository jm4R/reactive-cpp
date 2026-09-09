#include <circle/reactive/property.hpp>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

// Separate executable: allocation tracking must not include Catch's
// bookkeeping.
namespace {
std::size_t allocations{};

template <typename T>
struct original_property_layout
{
    T value{};
    circle::value_provider_ptr<T> provider;
    circle::scoped_connection unused_provider_observer;
    circle::signal<circle::property<T>&> changing, changed, moved, destroyed;
    bool changed_by_provider{};
};

template <typename T>
constexpr bool fits_original =
    sizeof(circle::property<T>) <= sizeof(original_property_layout<T>);

static_assert(fits_original<bool> && fits_original<int> &&
              fits_original<float> && fits_original<double> &&
              fits_original<std::string>);
static_assert(sizeof(circle::signal<>) == sizeof(std::shared_ptr<void>));
static_assert(sizeof(circle::interceptor_handle<int>) ==
              sizeof(std::weak_ptr<void>));
} // namespace

void* operator new(std::size_t size)
{
    if (auto* p = std::malloc(size ? size : 1))
    {
        ++allocations;
        return p;
    }
    throw std::bad_alloc{};
}
void* operator new[](std::size_t size)
{
    return ::operator new(size);
}
void operator delete(void* p) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept
{
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept
{
    std::free(p);
}

int main()
{
    const auto initial = allocations;
    {
        circle::signal<> s;
        s.emit();
        circle::property<int> p;
        circle::property<int> initialized{42};
        p = 1;
        p = 2;
        initialized = 43;
        circle::property<int> moved{std::move(p)};
        initialized = std::move(moved);
    }
    if (allocations != initial)
    {
        std::fputs("Unconnected scalar properties/signals allocated\n", stderr);
        return 1;
    }

    circle::property<int> p;
    int changing = 0, changed = 0;
    p.value_changing().connect([&] { ++changing; });
    p.value_changed().connect([&] { ++changed; });
    const auto connected = allocations;
    p = 1;
    p = 2;
    if (allocations != connected || changing != 2 || changed != 2)
    {
        std::fputs("Scalar publication allocated or missed notifications\n",
                   stderr);
        return 1;
    }
    // weak_ptr captures do not fit std::function's no-allocation path in
    // libstdc++; copying this callback on every request would fail this check.
    auto token = std::make_shared<int>(0);
    auto channel = p.intercept(
        [weak = std::weak_ptr<int>{token}](circle::change_request<int>& r) {
            if (auto state = weak.lock())
                ++*state;
            if (r.value() % 2)
                r.defer();
        });
    const auto intercepted = allocations;
    p = 3;
    const bool deferred = *p == 2;
    p = 4;
    channel.publish(5);
    if (allocations != intercepted || !deferred || *p != 5 || *token != 2)
    {
        std::fputs("Interception allocated or changed request semantics\n",
                   stderr);
        return 1;
    }
    std::puts("Property/signal size and zero-allocation checks passed");
}
