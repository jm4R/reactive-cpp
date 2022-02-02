#pragma once

#include <circle/bind/property.hpp>

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
        : changed_connections_{connect_changed(props)...},
          moved_connections_{connect_moved(props)...}
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

private:
    template <typename T>
    connection connect_changed(property<T>& p)
    {
        // TODO: support move
        return p.value_changed().connect([this]() { on_changed(); });
    }

    template <typename T>
    connection connect_moved(property<T>& p)
    {
        return p.moved().connect([this](property<T>& p) { on_moved(p); });
    }

    void on_changed()
    {
        if (callback_)
            callback_();
    }

    template <typename T>
    void on_moved(property<T>& prop)
    {
        for (scoped_connection& c : changed_connections_)
        {
            if (prop.value_changed().owns(c.get()))
            {
                c.disconnect();
                c = connect_changed(prop);
                break;
            }
        }
    }

private:
    std::function<void()> callback_;
    scoped_connection changed_connections_[sizeof...(Props)];
    scoped_connection moved_connections_[sizeof...(Props)];
};

template <typename... Args>
properties_observer(Args&...) -> properties_observer<Args...>;

} // namespace circle