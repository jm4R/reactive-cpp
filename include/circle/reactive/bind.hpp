#pragma once

#include <circle/reactive/observer.hpp>
#include <circle/reactive/property.hpp>

#include <memory>
#include <tuple>

namespace circle {

namespace detail {
template <typename T>
struct tracking_trait
{
    template <typename T1>
    static T1 element(const enable_tracking_ptr<T1>&)
    {
    }
    template <typename T1>
    static T1 element(const tracking_ptr<T1>&)
    {
    }
    template <typename T1>
    static T1 element(const property<T1>&)
    {
    }

    template <typename T1>
    static tracking_ptr<T1> ptr(enable_tracking_ptr<T1>& v)
    {
    }
    template <typename T1>
    static tracking_ptr<T1> ptr(tracking_ptr<T1>& v)
    {
    }
    template <typename T1>
    static property_ptr<T1> ptr(property<T1>& v)
    {
    }

    template <typename T1>
    static T1* make_ptr(T1& v)
    {
        return &v;
    }
    template <typename T1>
    static tracking_ptr<T1>& make_ptr(tracking_ptr<T1>& v)
    {
        return v;
    }

    using element_type = decltype(element(std::declval<T>()));
    using ptr_type = decltype(ptr(std::declval<T&>()));
};

template <typename T>
using tt = typename tracking_trait<T>::element_type;

template <typename T>
struct tracking_deref
{
    using element_type = typename tracking_trait<T>::element_type;
    using ptr_type = typename tracking_trait<T>::ptr_type;

    tracking_deref(T& v) : ptr{tracking_trait<T>::make_ptr(v)} {}
    ptr_type ptr;
    operator const element_type&() { return *ptr; }
};

} // namespace detail

template <typename T, typename... Args>
class binding : public value_provider<T>
{
public:
    binding(Args&... props, T (*f)(const detail::tt<Args>&...))
        : arguments_{detail::tracking_deref<Args>{props}...},
          function_{f},
          observer_{detail::tracking_trait<Args>::make_ptr(props)...}
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
    std::tuple<detail::tracking_deref<Args>...> arguments_;
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
    const typename detail::tt<decltype(val)>& name
#define CIRCLE_DECLTYPE_ARG1(tuple) CIRCLE_DECLTYPE_ARG2 tuple
#define CIRCLE_DECLTYPE_ARGS(...)                                              \
    CIRCLE_FOLD(CIRCLE_DECLTYPE_ARG1, CIRCLE_IGNORE, __VA_ARGS__)

#define CIRCLE_GET_LAST(...)                                                   \
    CIRCLE_FOR_EACH(CIRCLE_IGNORE, CIRCLE_IDENTITY, __VA_ARGS__)

#define BIND_IMPL(...)                                                         \
    make_binding(                                                              \
        +[](CIRCLE_DECLTYPE_ARGS(__VA_ARGS__)) {                               \
            return CIRCLE_GET_LAST(__VA_ARGS__);                               \
        },                                                                     \
        CIRCLE_TAKE_VALUES(__VA_ARGS__));

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

/* BIND macro resolution steps:
 *
 * -example:
 * BIND(arg1, (arg2, name2), expr)
 *
 * - BIND expands to:
 * BIND_IMPL(
 *   CIRCLE_FOLD(CIRCLE_MAKE_TUPLES, CIRCLE_ADD_PARAM, arg1, (arg2, name2),
 * expr)
 * )
 *
 * - CIRCLE_FOLD & CIRCLE_MAKE_TUPLES expands to:
 * BIND_IMPL(
 *   CIRCLE_IF_TUPLE(arg1, CIRCLE_IDENTITY, CIRCLE_MAKE_IDENTITY_TUPLE),
 *   CIRCLE_IF_TUPLE((arg2, name2), CIRCLE_IDENTITY,
 * CIRCLE_MAKE_IDENTITY_TUPLE), expr
 *   )
 * )
 *
 * - CIRCLE_IF_TUPLE expands to:
 * BIND_IMPL(
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     arg1,
 *     CIRCLE_UNPACK_IF_PAIR arg1,
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE
 *   )(arg1),
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     (arg2, name2),
 *     CIRCLE_UNPACK_IF_PAIR(arg2, name2),
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE
 *   )((arg2, name2)),
 *   expr
 *   )
 * )
 *
 * - CIRCLE_UNPACK_IF_PAIR expands to:
 * BIND_IMPL(
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     arg1,
 *     CIRCLE_UNPACK_IF_PAIR arg1,
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE
 *   )(arg1),
 *   CIRCLE_CHOOSE_FOR_TUPLE(
 *     (arg2, name2),
 *     arg2,
 *     name2,
 *     CIRCLE_IDENTITY,
 *     CIRCLE_MAKE_IDENTITY_TUPLE
 *   )((arg2, name2)),
 *   expr
 *   )
 * )
 *
 * - CIRCLE_CHOOSE_FOR_TUPLE expands to:
 * BIND_IMPL(
 *   CIRCLE_MAKE_IDENTITY_TUPLE (arg1),
 *   CIRCLE_IDENTITY((arg2, name2)),
 *   expr
 *   )
 * )
 *
 * - CIRCLE_MAKE_IDENTITY_TUPLE & CIRCLE_IDENTITY expands to:
 * BIND_IMPL(
 *   (arg1, arg1),
 *   (arg2, name2),
 *   expr
 *   )
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
