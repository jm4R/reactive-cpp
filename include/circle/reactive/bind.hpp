#pragma once

#include <circle/reactive/observer.hpp>
#include <circle/reactive/property.hpp>
#include <circle/reactive/ptr.hpp>

#include <memory>
#include <tuple>

namespace circle {

namespace detail {

template <typename T>
struct deref_impl;

template <typename T>
struct deref_impl<::circle::property<T>>
{
    using property_type = ::circle::property<T>;
    using value_type = typename property_type::value_type;

    deref_impl(const property_type& v) : ptr_{v} {}
    property_ref<value_type> ptr_;
    operator const value_type&() { return *ptr_; }
};

template <typename T>
struct deref_impl<::circle::property_ref<T>>
{
    using property_type = ::circle::property_ref<T>;
    using value_type = typename property_type::value_type;

    deref_impl(const property_type& v) : ptr_{v} {}
    property_ref<value_type> ptr_;
    operator const value_type&() { return *ptr_; }
};

template <typename T>
struct deref_impl<::circle::ptr<T>>
{
    using property_type = ::circle::ptr<T>;
    using value_type = typename property_type::value_type;

    deref_impl(const property_type& v) : ptr_{v} {}
    ::circle::tracking_ptr<value_type> ptr_;
    operator const value_type&() { return *ptr_; }
};

template <typename T>
struct deref_impl<::circle::tracking_ptr<T>>
{
    using property_type = ::circle::tracking_ptr<T>;
    using value_type = typename property_type::value_type;

    deref_impl(const property_type& v) : ptr_{v} {}
    ::circle::tracking_ptr<value_type> ptr_;
    operator const value_type&() { return *ptr_; }
};

template <typename T>
using deref = deref_impl<std::remove_cv_t<T>>;

template <typename T>
using tt = typename std::remove_reference_t<T>::value_type;

} // namespace detail

template <typename T, typename... Args>
class binding : public value_provider<T>
{
public:
    binding(Args&... props, T (*f)(const detail::tt<Args>&...))
        : arguments_{detail::deref<Args>{props}...},
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
    using function_t = T (*)(const detail::tt<Args>&...);
    std::tuple<detail::deref<Args>...> arguments_;
    function_t function_;
    observer<sizeof...(Args)> observer_;
    std::function<void()> updated_;
    std::function<void()> before_invalid_;
};

template <typename T, typename... Args>
using binding_ptr = std::unique_ptr<binding<T, Args...>>;

// This won't work with MSVC :(
// template <typename T, typename... Args>
// inline auto make_binding(T (*f)(const detail::tt<Args>&...), Args&... props)
//    -> value_provider_ptr<T>
//{
//    return std::make_unique<binding<T, Args...>>(props..., f);
//}

template <typename F, typename... Args>
inline auto make_binding(F* f, Args&... props)
{
    using R = std::invoke_result_t<F, const detail::tt<Args>&...>;
    using B = value_provider_ptr<R>;
    return static_cast<B&&>(std::make_unique<binding<R, Args...>>(props..., f));
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
#define CIRCLE_ADD_PARAM(x) , x

#define CIRCLE_TAKE_VALUE2(val, name) val
#define CIRCLE_TAKE_VALUE1(tuple) CIRCLE_TAKE_VALUE2 tuple
#define CIRCLE_TAKE_VALUES(...)                                                \
    CIRCLE_FOLD(CIRCLE_TAKE_VALUE1, CIRCLE_IGNORE, __VA_ARGS__)

#define CIRCLE_DECLTYPE_ARG2(val, name)                                        \
    const typename ::circle::detail::tt<decltype(val)>& name
#define CIRCLE_DECLTYPE_ARG1(tuple) CIRCLE_DECLTYPE_ARG2 tuple
#define CIRCLE_DECLTYPE_ARGS(...)                                              \
    CIRCLE_FOLD(CIRCLE_DECLTYPE_ARG1, CIRCLE_IGNORE, __VA_ARGS__)

#define CIRCLE_GET_LAST(...)                                                   \
    CIRCLE_FOR_EACH(CIRCLE_IGNORE, CIRCLE_IDENTITY, __VA_ARGS__)

#define BIND_IMPL(...)                                                         \
    ::circle::make_binding(                                                    \
        +[](CIRCLE_DECLTYPE_ARGS(__VA_ARGS__)) {                               \
            return CIRCLE_GET_LAST(__VA_ARGS__);                               \
        },                                                                     \
        CIRCLE_TAKE_VALUES(__VA_ARGS__))

#define CIRCLE_UNPACK_IF_PAIR(x, y) x, y
#define CIRCLE_CHOOSE_FOR_TUPLE_IMPL(arg1, arg2_or_tuple_macro,                \
                                     tuple_or_non_tuple_macro, ...)            \
    tuple_or_non_tuple_macro
#define CIRCLE_CHOOSE_FOR_TUPLE(...)                                           \
    CIRCLE_MSVC_EXP(CIRCLE_CHOOSE_FOR_TUPLE_IMPL(__VA_ARGS__))
#define CIRCLE_IF_TUPLE(v, tuple_macro, non_tuple_macro)                       \
    CIRCLE_CHOOSE_FOR_TUPLE(CIRCLE_UNPACK_IF_PAIR v, tuple_macro,              \
                            non_tuple_macro, 0)                                \
    (v)

#define CIRCLE_MAKE_IDENTITY_TUPLE(x) (x, x)
#define CIRCLE_MAKE_TUPLES(x)                                                  \
    CIRCLE_IF_TUPLE(x, CIRCLE_IDENTITY, CIRCLE_MAKE_IDENTITY_TUPLE)
#define BIND(...)                                                              \
    BIND_IMPL(CIRCLE_FOLD(CIRCLE_MAKE_TUPLES, CIRCLE_ADD_PARAM, __VA_ARGS__))
#define BIND_EQ(x) BIND((x, n), n)

/* BIND macro resolution steps:
 *
 * -example:
 * BIND(arg1, (arg2, name2), expr)
 *
 * - BIND expands to:
 * BIND_IMPL(
 *   CIRCLE_FOLD(CIRCLE_MAKE_TUPLES, CIRCLE_ADD_PARAM, arg1, (arg2, name2),
 *               expr)
 * )
 *
 * - CIRCLE_FOLD & CIRCLE_MAKE_TUPLES expands to:
 * BIND_IMPL(
 *   CIRCLE_IF_TUPLE(arg1, CIRCLE_IDENTITY, CIRCLE_MAKE_IDENTITY_TUPLE),
 *   CIRCLE_IF_TUPLE((arg2, name2), CIRCLE_IDENTITY,
 *                   CIRCLE_MAKE_IDENTITY_TUPLE),
 *   expr
 * )
 *
 * - CIRCLE_IF_TUPLE expands to:
 * BIND_IMPL(
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     CIRCLE_UNPACK_IF_PAIR arg1,
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE,
 *     0
 *   )(arg1),
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     CIRCLE_UNPACK_IF_PAIR(arg2, name2),
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE,
 *     0
 *   )((arg2, name2)),
 *   expr
 * )
 *
 * - CIRCLE_UNPACK_IF_PAIR expands to:
 * BIND_IMPL(
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     CIRCLE_UNPACK_IF_PAIR arg1,
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE,
 *     0
 *   )(arg1),
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     arg2,
 *     name2,
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE,
 *     0
 *   )((arg2, name2)),
 *   expr
 * )
 *
 * - CIRCLE_CHOOSE_FOR_TUPLE expands to:
 * BIND_IMPL(
 *   CIRCLE_MAKE_IDENTITY_TUPLE (arg1),
 *   CIRCLE_IDENTITY((arg2, name2)),
 *   expr
 * )
 *
 * - CIRCLE_MAKE_IDENTITY_TUPLE & CIRCLE_IDENTITY expands to:
 * BIND_IMPL(
 *   (arg1, arg1),
 *   (arg2, name2),
 *   expr
 * )
 *
 * - BIND_IMPL expands to:
 * make_binding(
 *   +[]
 *   (
 *     const typename detail::tt<decltype(arg1)>& arg1,
 *     const typename detail::tt<decltype(arg2)>& name2
 *   )
 *   {
 *     return expr;
 *   },
 *   arg1,
 *   arg2
 * );
 *
 * */
} // namespace circle
