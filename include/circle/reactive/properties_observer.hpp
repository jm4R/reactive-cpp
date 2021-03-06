#pragma once

#include <circle/reactive/property.hpp>

#include <functional>
#include <type_traits>

namespace circle {

template <typename... Props>
class properties_observer
{
public:
    template <typename... LArgs,
              typename = std::enable_if_t<
                  std::conjunction_v<is_property<LArgs>...>, void>>
    properties_observer(LArgs&... props)
        : changed_connections_{connect_changed(props)...}, destroyed_connections_{connect_destroyed(props)...}
    {
    }

    properties_observer(const properties_observer&) = delete;
    properties_observer& operator=(const properties_observer&) = delete;
    properties_observer(properties_observer&& other) = delete;
    properties_observer& operator=(properties_observer&&) = delete;

    void set_callback(std::function<void()> callback)
    {
        callback_ = std::move(callback);
    }

    void set_destroyed_callback(std::function<void()> callback)
    {
        destroyed_callback_ = std::move(callback);
    }

private:
    template <typename T>
    connection connect_changed(property<T>& p)
    {
        return p.value_changed().connect(&properties_observer::on_changed, this);
    }

    template <typename T>
    connection connect_destroyed(property<T>& p)
    {
        return p.before_destroyed().connect(&properties_observer::on_destroyed, this);
    }

    void on_changed()
    {
        if (callback_)
            callback_();
    }

    void on_destroyed()
    {
        if (destroyed_callback_)
            destroyed_callback_();
    }

private:
    std::function<void()> callback_;
    std::function<void()> destroyed_callback_;
    // be aware: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80438
    static_assert(sizeof...(Props) != 0,
                  "Can't create empty property observer");
    scoped_connection changed_connections_[sizeof...(Props)];
    scoped_connection destroyed_connections_[sizeof...(Props)];
};

template <typename... Args>
properties_observer(property<Args>&...)
    -> properties_observer<property<Args>...>;

} // namespace circle