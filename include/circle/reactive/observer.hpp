#pragma once

#include <circle/reactive/signal.hpp>

#include <functional>
#include <type_traits>

namespace circle {

namespace detail {

template <typename T, typename = void>
struct has_value_changing_signal : std::false_type
{
};
template <typename T>
struct has_value_changing_signal<
    T, std::enable_if_t<is_signal<
           std::decay_t<decltype(std::declval<T&>().value_changing())>>::value>>
    : std::true_type
{
};
template <typename T, typename = void>
struct has_value_changed_signal : std::false_type
{
};
template <typename T>
struct has_value_changed_signal<
    T, std::enable_if_t<is_signal<
           std::decay_t<decltype(std::declval<T&>().value_changed())>>::value>>
    : std::true_type
{
};
template <typename T>
inline constexpr bool has_value_changing_signals_v =
    has_value_changing_signal<T>::value && has_value_changed_signal<T>::value;

} // namespace detail

template <std::size_t N>
class observer
{
public:
    template <typename... LArgs>
    observer(LArgs&... props)
        : changing_connections_{connect_changing(props)...},
          changed_connections_{connect_changed(props)...},
          destroyed_connections_{connect_destroyed(props)...}
    {
    }

    observer(const observer&) = delete;
    observer& operator=(const observer&) = delete;
    observer(observer&& other) = delete;
    observer& operator=(observer&&) = delete;

    void set_changing_callback(std::function<void()> callback)
    {
        changing_callback_ = std::move(callback);
    }

    void set_changed_callback(std::function<void()> callback)
    {
        changed_callback_ = std::move(callback);
    }

    void set_destroyed_callback(std::function<void()> callback)
    {
        destroyed_callback_ = std::move(callback);
    }

private:
    template <typename T>
    connection connect_changing(T& p)
    {
        if constexpr (detail::has_value_changing_signals_v<T>)
        {
            return p.value_changing().connect(&observer::on_changing, this);
        }
        else
        {
            return connection{};
        }
    }

    template <typename T>
    connection connect_changed(T& p)
    {
        if constexpr (detail::has_value_changing_signals_v<T>)
        {
            return p.value_changed().connect(&observer::on_changed, this);
        }
        else
        {
            return connection{};
        }
    }

    template <typename T>
    connection connect_destroyed(T& p)
    {
        return p.before_destroyed().connect(&observer::on_destroyed, this);
    }

    void on_changing()
    {
        if (changing_callback_)
            changing_callback_();
    }

    void on_changed()
    {
        if (changed_callback_)
            changed_callback_();
    }

    void on_destroyed()
    {
        if (destroyed_callback_)
            destroyed_callback_();
    }

private:
    std::function<void()> changing_callback_;
    std::function<void()> changed_callback_;
    std::function<void()> destroyed_callback_;
    // be aware: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80438
    static_assert(N != 0, "Can't create empty observer");
    scoped_connection changing_connections_[N];
    scoped_connection changed_connections_[N];
    scoped_connection destroyed_connections_[N];
};

template <typename... Args>
observer(Args&...) -> observer<sizeof...(Args)>;

} // namespace circle