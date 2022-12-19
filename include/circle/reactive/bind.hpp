#pragma once

#include <circle/reactive/properties_observer.hpp>
#include <circle/reactive/property.hpp>

#include <memory>
#include <tuple>

namespace circle {

namespace detail {
template <typename T>
struct property_deref
{
    property_ptr<T> ref;
    operator const T&() { return *ref; }
};
} // namespace detail

template <typename T, typename... Args>
class binding : public value_provider<T>
{
public:
    binding(property<Args>&... props, T (*f)(const Args&...))
        : arguments_{detail::property_deref<Args>{&props}...},
          function_{f},
          observer_{props...}
    {
        observer_.set_callback([this] { updated_(); });
        observer_.set_destroyed_callback([this] { before_invalid_(); });
    }

    ~binding() override = default;

    binding(binding&&) = delete;
    binding& operator=(binding&&) = delete;

    binding(const binding&) = delete;
    binding& operator=(const binding&) = delete;

public:
    void set_updated_callback(std::function<void()> clb) override
    {
        updated_ = std::move(clb);
    }
    void set_before_invalid_callback(std::function<void()> clb) override
    {
        before_invalid_ = std::move(clb);
    }
    T get() override { return std::apply(function_, arguments_); }

private:
    using function_t = T (*)(const Args&...);
    std::tuple<detail::property_deref<Args>...> arguments_;
    function_t function_;
    properties_observer<property<Args>...> observer_;
    std::function<void()> updated_;
    std::function<void()> before_invalid_;
};

template <typename T, typename... Args>
using binding_ptr = std::unique_ptr<binding<T, Args...>>;

template <typename T, typename... Args>
inline auto make_binding(T (*f)(const Args&...), property<Args>&... props)
    -> value_provider_ptr<T>
{
    return std::make_unique<binding<T, Args...>>(props..., f);
}

// workaround for old msvc preprocessor:
// https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define CIRCLE_MSVC_EXP(x) x

// clang-format off
#define CIRCLE_FO_0(f, last)
#define CIRCLE_FO_1(f, last, x) last(x)
#define CIRCLE_FO_2(f, last, x, ...) f(x)   CIRCLE_MSVC_EXP(CIRCLE_FO_1(f, last, __VA_ARGS__))
#define CIRCLE_FO_3(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_2(f, last, __VA_ARGS__))
#define CIRCLE_FO_4(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_3(f, last, __VA_ARGS__))
#define CIRCLE_FO_5(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_4(f, last, __VA_ARGS__))
#define CIRCLE_FO_6(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_5(f, last, __VA_ARGS__))
#define CIRCLE_FO_7(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_6(f, last, __VA_ARGS__))
#define CIRCLE_FO_8(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_7(f, last, __VA_ARGS__))
#define CIRCLE_FO_9(f, last, x, ...) f(x),  CIRCLE_MSVC_EXP(CIRCLE_FO_8(f, last, __VA_ARGS__))
#define CIRCLE_FO_10(f, last, x, ...) f(x), CIRCLE_MSVC_EXP(CIRCLE_FO_9(f, last, __VA_ARGS__))
#define CIRCLE_FO_11(f, last, x, ...) f(x), CIRCLE_MSVC_EXP(CIRCLE_FO_10(f, last, __VA_ARGS__))
#define CIRCLE_FO_12(f, last, x, ...) f(x), CIRCLE_MSVC_EXP(CIRCLE_FO_11(f, last, __VA_ARGS__))
#define CIRCLE_FO_13(f, last, x, ...) f(x), CIRCLE_MSVC_EXP(CIRCLE_FO_12(f, last, __VA_ARGS__))
#define CIRCLE_FO_14(f, last, x, ...) f(x), CIRCLE_MSVC_EXP(CIRCLE_FO_13(f, last, __VA_ARGS__))
#define CIRCLE_FO_15(f, last, x, ...) f(x), CIRCLE_MSVC_EXP(CIRCLE_FO_14(f, last, __VA_ARGS__))

#define CIRCLE_FE_0(f, last)
#define CIRCLE_FE_1(f, last, x) last(x)
#define CIRCLE_FE_2(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_1(f, last, __VA_ARGS__))
#define CIRCLE_FE_3(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_2(f, last, __VA_ARGS__))
#define CIRCLE_FE_4(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_3(f, last, __VA_ARGS__))
#define CIRCLE_FE_5(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_4(f, last, __VA_ARGS__))
#define CIRCLE_FE_6(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_5(f, last, __VA_ARGS__))
#define CIRCLE_FE_7(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_6(f, last, __VA_ARGS__))
#define CIRCLE_FE_8(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_7(f, last, __VA_ARGS__))
#define CIRCLE_FE_9(f, last, x, ...) f(x)  CIRCLE_MSVC_EXP(CIRCLE_FE_8(f, last, __VA_ARGS__))
#define CIRCLE_FE_10(f, last, x, ...) f(x) CIRCLE_MSVC_EXP(CIRCLE_FE_9(f, last, __VA_ARGS__))
#define CIRCLE_FE_11(f, last, x, ...) f(x) CIRCLE_MSVC_EXP(CIRCLE_FE_10(f, last, __VA_ARGS__))
#define CIRCLE_FE_12(f, last, x, ...) f(x) CIRCLE_MSVC_EXP(CIRCLE_FE_11(f, last, __VA_ARGS__))
#define CIRCLE_FE_13(f, last, x, ...) f(x) CIRCLE_MSVC_EXP(CIRCLE_FE_12(f, last, __VA_ARGS__))
#define CIRCLE_FE_14(f, last, x, ...) f(x) CIRCLE_MSVC_EXP(CIRCLE_FE_13(f, last, __VA_ARGS__))
#define CIRCLE_FE_15(f, last, x, ...) f(x) CIRCLE_MSVC_EXP(CIRCLE_FE_14(f, last, __VA_ARGS__))
// clang-format on

#define CIRCLE_MATCH_ARGS(ign0, ign1, ign2, ign3, ign4, ign5, ign6, ign7,      \
                          ign8, ign9, ign10, ign11, ign12, ign13, ign14,       \
                          ign15, name, ...)                                    \
    name

#define CIRCLE_FOLD(action, last, ...)                                         \
    CIRCLE_MSVC_EXP(CIRCLE_MATCH_ARGS(                                         \
        _0, __VA_ARGS__, CIRCLE_FO_15, CIRCLE_FO_14, CIRCLE_FO_13,             \
        CIRCLE_FO_12, CIRCLE_FO_11, CIRCLE_FO_10, CIRCLE_FO_9, CIRCLE_FO_8,    \
        CIRCLE_FO_7, CIRCLE_FO_6, CIRCLE_FO_5, CIRCLE_FO_4, CIRCLE_FO_3,       \
        CIRCLE_FO_2, CIRCLE_FO_1, CIRCLE_FO_0)(action, last, __VA_ARGS__))

#define CIRCLE_FOR_EACH(action, last, ...)                                     \
    CIRCLE_MSVC_EXP(CIRCLE_MATCH_ARGS(                                         \
        _0, __VA_ARGS__, CIRCLE_FE_15, CIRCLE_FE_14, CIRCLE_FE_13,             \
        CIRCLE_FE_12, CIRCLE_FE_11, CIRCLE_FE_10, CIRCLE_FE_9, CIRCLE_FE_8,    \
        CIRCLE_FE_7, CIRCLE_FE_6, CIRCLE_FE_5, CIRCLE_FE_4, CIRCLE_FE_3,       \
        CIRCLE_FE_2, CIRCLE_FE_1, CIRCLE_FE_0)(action, last, __VA_ARGS__))

#define CIRCLE_IGNORE(x)

#define CIRCLE_IDENTITY(x) x
#define CIRCLE_IDENTITIES(...)                                                 \
    CIRCLE_FOLD(CIRCLE_IDENTITY, CIRCLE_IGNORE, __VA_ARGS__)

#define CIRCLE_DECLTYPE_ARG(x) decltype(x.get()) x
#define CIRCLE_DECLTYPE_ARGS(...)                                              \
    CIRCLE_FOLD(CIRCLE_DECLTYPE_ARG, CIRCLE_IGNORE, __VA_ARGS__)

#define CIRCLE_GET_LAST(...)                                                   \
    CIRCLE_FOR_EACH(CIRCLE_IGNORE, CIRCLE_IDENTITY, __VA_ARGS__)

#define BIND(...)                                                              \
    make_binding(                                                              \
        +[](CIRCLE_DECLTYPE_ARGS(__VA_ARGS__)) {                               \
            return CIRCLE_GET_LAST(__VA_ARGS__);                               \
        },                                                                     \
        CIRCLE_IDENTITIES(__VA_ARGS__));

} // namespace circle
