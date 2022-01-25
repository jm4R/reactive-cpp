#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>
//#include <stdexcept>

namespace circle {

namespace detail {

template <std::size_t... Is, typename F, typename Tuple>
constexpr void invoke_impl(
    std::index_sequence<Is...>, F&& f, Tuple&& args,
    std::enable_if_t<std::is_invocable_v<F, std::tuple_element_t<Is, Tuple>...>,
                     void>* = nullptr)
{
    std::invoke(std::forward<F>(f), std::get<Is>(args)...);
}

template <std::size_t... Is, typename F, typename Tuple>
constexpr void invoke_impl(
    std::index_sequence<Is...>, F&& f, Tuple&& args,
    std::enable_if_t<
        !std::is_invocable_v<F, std::tuple_element_t<Is, Tuple>...>, void>* =
        nullptr)
{
    static_assert(sizeof...(Is) > 0, "Not invocable with arguments supplied");
    if constexpr (sizeof...(Is) > 0)
    {
        invoke_impl(std::make_index_sequence<sizeof...(Is) - 1>(),
                    std::forward<F>(f), std::move(args));
    }
}

// invokes function with possibly too many parameters provided
template <typename F, typename... Args>
constexpr void invoke(F&& f, Args&&... args)
{
    invoke_impl(std::make_index_sequence<sizeof...(Args)>(), std::forward<F>(f),
                std::forward_as_tuple(std::forward<Args>(args)...));
}

template <typename T>
constexpr T fail(T val, const char* reason)
{
    // throw std::exception(reason);
    return val;
}

enum class id : unsigned
{
};

class connections_container_base
{
public:
    virtual void disconnect(id connection_id) = 0;
    [[nodiscard]] virtual bool active(id connection_id) const = 0;
    virtual bool block(id connection_id, bool) = 0;
    [[nodiscard]] virtual bool blocked(id connection_id) const = 0;

protected:
    ~connections_container_base() = default;
};

template <typename... Args>
class connections_container : public connections_container_base
{
    using slot_type = std::function<void(Args...)>;
    struct connection_data
    {
        detail::id id;
        slot_type slot;
        bool blocked{};
    };

    const connection_data* find_impl(id id) const
    {
        const auto it =
            std::lower_bound(connections_.begin(), connections_.end(), id,
                             [](auto l, auto v) { return l.id < v; });
        return (it != connections_.end() && it->id == id)
                   ? &(*it)
                   : static_cast<connection_data*>(nullptr);
    }

    const connection_data* find(id id) const { return find_impl(id); }

    connection_data* find(id id)
    {
        return const_cast<connection_data*>(find_impl(id));
    }

    detail::id next_id()
    {
        auto val = static_cast<std::underlying_type_t<detail::id>>(next_id_);
        assert(val < 0xffffffff);
        next_id_ = id{val + 1};
        return detail::id{val};
    }

public:
    template <typename... LArgs>
    void invoke(LArgs&&... largs)
    {
        for (auto c : connections_)
        {
            if (c.slot && !c.blocked)
                c.slot(std::forward<LArgs>(largs)...);
        }
    }

    id connect(slot_type s)
    {
        const auto id = next_id();
        connections_.push_back(connection_data{id, std::move(s)});
        return id;
    }

    void disconnect_all()
    {
        connections_ = {};
        next_id_ = id{1};
    }

    void disconnect(id connection_id) override
    {
        if (auto c = find(connection_id))
        {
            c->slot = {};
        }
    }

    [[nodiscard]] bool active(id connection_id) const override
    {
        if (auto c = find(connection_id))
        {
            return !!c->slot;
        }
        return false;
    }

    bool block(id connection_id, bool val) override
    {
        if (auto c = find(connection_id))
        {
            if (!c->slot)
                return detail::fail(false, "Blocking inactive connection");
            return std::exchange(c->blocked, val);
        }
        return detail::fail(false, "Blocking inactive connection");
    }

    [[nodiscard]] bool blocked(id connection_id) const override
    {
        if (auto c = find(connection_id))
        {
            if (!c->slot)
                return detail::fail(false,
                                    "Checking inactive connection if blocked");
            return c->blocked;
        }
        return detail::fail(false, "Checking inactive connection if blocked");
    }

private:
    detail::id next_id_{1};
    std::vector<connection_data> connections_;
};

} // namespace detail

class connection
{
    template <typename... Args>
    friend class signal;

public:
    connection() = default;

    [[nodiscard]] bool active() const
    {
        if (auto s = connections_.lock())
        {
            return s->active(id_);
        }
        return false;
    }

    void disconnect()
    {
        if (auto s = connections_.lock())
        {
            s->disconnect(id_);
        }
    }

    template <typename T>
    [[nodiscard]] bool belongs_to(const T& v)
    {
        return v.owns_connection(*this);
    }

    bool block(bool val)
    {
        if (auto s = connections_.lock())
        {
            return s->block(id_, val);
        }
        return detail::fail(false, "Blocking inactive connection");
    }

    [[nodiscard]] bool blocked() const
    {
        if (auto s = connections_.lock())
        {
            return s->blocked(id_);
        }
        return detail::fail(false, "Checking inactive connection if blocked");
    }

private:
    connection(std::weak_ptr<detail::connections_container_base> conections,
               detail::id id)
        : connections_{std::move(conections)}, id_{id}
    {
    }

private:
    std::weak_ptr<detail::connections_container_base> connections_;
    detail::id id_;
};

struct connection_blocker
{
    connection_blocker(connection c) : connection_{c}
    {
        if (!c.active())
            detail::fail(false,
                         "Creating connection_blocker on inactive connection");
        was_ = c.block(true);
    }

    ~connection_blocker() { connection_.block(was_); }

    bool was_;
    connection connection_;
};

template <typename... Args>
class signal
{
    using connections_type = detail::connections_container<Args...>;
    using slot_type = std::function<void(Args...)>;

public:
    signal() = default;

    signal(const signal&) = delete;
    signal& operator=(const signal&) = delete;

    signal(signal&&) = default;
    signal& operator=(signal&&) = default;

    template <typename... LArgs>
    void emit(LArgs&&... largs) const
    {
        if (!connections_)
            return;

        connections_->invoke(std::forward<LArgs>(largs)...);
    }

    template <typename... LArgs>
    void operator()(LArgs&&... largs) const
    {
        if (!connections_)
            return;

        connections_->invoke(std::forward<LArgs>(largs)...);
    }

    template <typename T, typename R>
    connection connect_mem(R (T::*memfun)(Args...), T* obj)
    {
        slot_type s = [memfun, obj](Args... args) {
            detail::invoke(memfun, obj, args...);
        };
        return connect_fun(std::move(s));
    }

    connection connect(slot_type f) { return connect_fun(std::move(f)); }

    template <typename F, typename... LArgs>
    connection connect(F&& f, LArgs&&... largs)
    {
        slot_type s = [f = std::forward<F>(f), largs...](Args... args) mutable {
            detail::invoke(f, largs..., args...);
        };
        return connect_fun(std::move(s));
    }

    template <typename F>
    connection operator+=(F&& f)
    {
        return connect(std::forward<F>(f));
    }

    void disconnect_all()
    {
        if (connections_)
            connections_->disconnect_all();
    }

    bool block(connection c, bool v) { return c.block(v); }

    [[nodiscard]] bool blocked(connection c) const { return c.blocked(); }

    void disconnect(connection c) { c.disconnect(); }

    [[nodiscard]] bool owns(connection c) const
    {
        return c.active() && c.connections_.lock() == connections_;
    }

private:
    connection connect_fun(std::function<void(Args...)> f)
    {
        if (!connections_)
            connections_ = std::make_shared<connections_type>();
        return {connections_, connections_->connect(std::move(f))};
    }

private:
    std::shared_ptr<connections_type> connections_;
};

template<typename... Args>
struct overload_impl
{
    template <typename R, typename T>
    using type = R (T::*)(Args...);
};

template <typename... Args>
using overload = typename overload_impl<Args...>::type;

} // namespace circle