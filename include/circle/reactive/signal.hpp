#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef NDEBUG
#    define CIRCLE_WARN(reason)
#    define CIRCLE_WARN_VAL(res, reason) (res)
#else
#    include <cstdio>
#    define CIRCLE_WARN(reason) fputs("WARNING: " reason "\n", stderr)
#    define CIRCLE_WARN_VAL(res, reason)                                       \
        (fputs("WARNING: " reason "\n", stderr), res)
#endif

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

struct increment_guard
{
    increment_guard(unsigned& val) noexcept : val_{val} { ++val_; }
    ~increment_guard() { --val_; }

private:
    unsigned& val_;
};

enum class id : unsigned
{
};

class connections_container_base
{
public:
    virtual void disconnect(id connection_id) noexcept = 0;
    [[nodiscard]] virtual bool active(id connection_id) const noexcept = 0;
    virtual bool block(id connection_id, bool) noexcept = 0;
    [[nodiscard]] virtual bool blocked(id connection_id) const noexcept = 0;

protected:
    ~connections_container_base() = default;
};

template <typename... Args>
class connections_container final : public connections_container_base
{
    using slot_type = std::function<void(Args...)>;
    struct connection_data
    {
        detail::id id;
        slot_type slot;
        unsigned invoke_depth{};
        bool blocked{};
        bool lazy_disconnect{};
    };

    const connection_data* find_impl(id connection_id) const noexcept
    {
        auto find_in = [&](const std::vector<connection_data>& v) {
            auto it = std::lower_bound(v.begin(), v.end(), connection_id,
                                       [](auto l, auto v) { return l.id < v; });
            return (it != v.end() && it->id == connection_id)
                       ? &(*it)
                       : static_cast<connection_data*>(nullptr);
        };

        if (auto found = find_in(connections_))
            return found;

        return find_in(new_connections_);
    }

    const connection_data* find(id id) const noexcept { return find_impl(id); }

    connection_data* find(id id) noexcept
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

    static bool active(const connection_data& c) noexcept
    {
        return !!c.slot && !c.lazy_disconnect;
    }

    static void disconnect(connection_data& c) noexcept
    {
        if (!c.invoke_depth)
        {
            c.slot = {};
        }
        else
        {
            c.lazy_disconnect = true;
        }
    }

public:
    template <typename... LArgs>
    void invoke(LArgs&&... largs)
    {
        increment_guard inv_depth_guard{iterations_depth_};
        for (auto& c : connections_)
        {
            if (active(c) && !c.blocked)
            {
                increment_guard conn_depth_guard{c.invoke_depth};
                c.slot(/*do_not_forward*/ largs...);
                if (c.lazy_disconnect && c.invoke_depth == 1)
                    c.slot = {};
            }
        }
        if (iterations_depth_ == 1)
            post_invoke();
    }

    id connect(slot_type s)
    {
        const auto id = next_id();
        if (!iterations_depth_)
            connections_.push_back(connection_data{id, std::move(s)});
        else
            new_connections_.push_back(connection_data{id, std::move(s)});
        return id;
    }

    void disconnect_all() noexcept
    {
        if (!iterations_depth_)
        {
            connections_ = {};
            new_connections_ = {};
        }
        else
        {
            for (auto& c : connections_)
            {
                disconnect(c);
            }
        }
    }

    void disconnect(id connection_id) noexcept override
    {
        if (!iterations_depth_)
        {
            const auto it = std::lower_bound(
                connections_.begin(), connections_.end(), connection_id,
                [](auto l, auto v) { return l.id < v; });

            if ((it != connections_.end() && it->id == connection_id))
                connections_.erase(it);
        }
        else
        {
            if (auto c = find(connection_id))
            {
                disconnect(*c);
            }
        }
    }

    [[nodiscard]] bool active(id connection_id) const noexcept override
    {
        if (auto c = find(connection_id))
        {
            return active(*c);
        }
        return false;
    }

    bool block(id connection_id, bool val) noexcept override
    {
        if (auto c = find(connection_id))
        {
            if (!active(*c))
                return CIRCLE_WARN_VAL(true, "Blocking inactive connection");
            return std::exchange(c->blocked, val);
        }
        return CIRCLE_WARN_VAL(true, "Blocking inactive connection");
    }

    [[nodiscard]] bool blocked(id connection_id) const noexcept override
    {
        if (auto c = find(connection_id))
        {
            if (!c->slot || c->lazy_disconnect)
                return CIRCLE_WARN_VAL(
                    true, "Checking inactive connection if blocked");
            return c->blocked;
        }
        return CIRCLE_WARN_VAL(true, "Checking inactive connection if blocked");
    }

private:
    void post_invoke()
    {
        const auto is_inactive = [](const connection_data& c) {
            return !c.slot;
        };
        connections_.erase(std::remove_if(connections_.begin(),
                                          connections_.end(), is_inactive),
                           connections_.end());
        connections_.insert(connections_.end(),
                            std::move_iterator{new_connections_.begin()},
                            std::move_iterator{new_connections_.end()});
        new_connections_.resize(0);
    }

private:
    detail::id next_id_{1};
    std::vector<connection_data> connections_;
    std::vector<connection_data> new_connections_;

    unsigned iterations_depth_{};
};

} // namespace detail

class connection
{
    template <typename... Args>
    friend class signal;

public:
    connection() = default;

    [[nodiscard]] bool active() const noexcept
    {
        if (auto s = connections_.lock())
        {
            return s->active(id_);
        }
        return false;
    }

    void disconnect() noexcept
    {
        if (auto s = connections_.lock())
        {
            s->disconnect(id_);
        }
    }

    template <typename T>
    [[nodiscard]] bool belongs_to(const T& v) noexcept
    {
        return v.owns(*this);
    }

    bool block(bool val) noexcept
    {
        if (auto s = connections_.lock())
        {
            return s->block(id_, val);
        }
        return CIRCLE_WARN_VAL(true, "Blocking inactive connection");
    }

    [[nodiscard]] bool blocked() const noexcept
    {
        if (auto s = connections_.lock())
        {
            return s->blocked(id_);
        }
        return CIRCLE_WARN_VAL(true, "Checking inactive connection if blocked");
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

class connection_blocker
{
public:
    connection_blocker(connection c) noexcept : connection_{c}
    {
        if (!c.active())
            CIRCLE_WARN("Creating connection_blocker on inactive connection");
        was_ = c.block(true);
    }

    connection_blocker(const connection_blocker&) = delete;
    connection_blocker& operator=(const connection_blocker&) = delete;
    connection_blocker(connection_blocker&&) = delete;
    connection_blocker operator==(connection_blocker&&) = delete;

    ~connection_blocker() { connection_.block(was_); }

private:
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
        emit(std::forward<LArgs>(largs)...);
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

    [[nodiscard]] bool owns(connection c) const noexcept
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

class scoped_connection
{
public:
    explicit scoped_connection() = default;

    scoped_connection(const scoped_connection&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;

    explicit scoped_connection(scoped_connection&& other) noexcept
        : c_{std::exchange(other.c_, {})}
    {
    }

    scoped_connection& operator=(scoped_connection&& other) noexcept
    {
        c_.disconnect();
        c_ = std::exchange(other.c_, {});
        return *this;
    }

    scoped_connection(connection src) noexcept : c_{std::move(src)} {}
    scoped_connection& operator=(connection src) noexcept
    {
        c_.disconnect();
        c_ = std::move(src);
        return *this;
    }
    ~scoped_connection() { c_.disconnect(); }

    connection& get() noexcept { return c_; }
    void release() noexcept { c_ = {}; }
    void disconnect() noexcept { c_.disconnect(); }

private:
    connection c_;
};

template <typename... Args>
struct is_signal : std::false_type
{
};

template <typename... Args>
struct is_signal<signal<Args...>> : std::true_type
{
};

} // namespace circle

#undef CIRCLE_WARN