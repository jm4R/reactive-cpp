#pragma once

#include <circle/reactive/signal.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace circle {

namespace detail {

template <typename T>
constexpr bool eq(const T& v1, const T& v2)
{
    if constexpr (std::equality_comparable<T>)
    {
        return v1 == v2;
    }
    else
    {
        return false;
    }
}

} // namespace detail

template <typename T>
class value_provider
{
public:
    virtual ~value_provider() {}
    virtual void set_updating_callback(std::function<void()> clb) = 0;
    virtual void set_updated_callback(std::function<void()> clb) = 0;
    virtual void set_before_invalid_callback(std::function<void()> clb) = 0;
    virtual T get() = 0;
};

template <typename T>
using value_provider_ptr = std::unique_ptr<value_provider<T>>;

template <typename T>
class change_request
{
public:
    const T& value() const noexcept { return value_; }
    void defer() noexcept { deferred_ = true; }
    bool deferred() const noexcept { return deferred_; }
    change_request(const change_request&) = delete;
    change_request& operator=(const change_request&) = delete;

private:
    template <typename>
    friend class property;
    explicit change_request(const T& value) noexcept : value_{value} {}
    const T& value_;
    bool deferred_{};
};

template <typename T>
class property;

namespace detail {
template <typename T>
struct interceptor_state
{
    property<T>* owner{};
    std::function<void(change_request<T>&)> request;
    std::function<void()> invalidated;
    bool active{true};
    std::size_t revision{};
    unsigned invoke_depth{};
};
} // namespace detail

template <typename T>
class interceptor_handle
{
public:
    interceptor_handle() = default;
    bool valid() const
    {
        auto s = state_.lock();
        return s && s->active && s->owner;
    }
    std::optional<T> current() const;
    bool publish(const T& value) const;
    void reset() const
    {
        auto s = state_.lock();
        if (s && s->active)
        {
            s->active = false;
            ++s->revision;
            if (!s->invoke_depth)
                s->request = {};
            auto invalidated = std::move(s->invalidated);
            if (invalidated)
                invalidated();
        }
    }

private:
    friend class property<T>;
    explicit interceptor_handle(
        const std::shared_ptr<detail::interceptor_state<T>>& s)
        : state_{s}
    {
    }
    std::weak_ptr<detail::interceptor_state<T>> state_;
};

template <typename T>
class property
{
    friend class interceptor_handle<T>;
    struct publication_guard
    {
        explicit publication_guard(property& p) noexcept
            : owner{&p}, previous{std::exchange(p.publication_guards_, this)}
        {
        }
        ~publication_guard()
        {
            if (owner)
            {
                assert(owner->publication_guards_ == this);
                owner->publication_guards_ = previous;
            }
        }
        publication_guard(const publication_guard&) = delete;
        publication_guard& operator=(const publication_guard&) = delete;
        property* owner;
        publication_guard* previous;
    };

    struct invocation_guard
    {
        explicit invocation_guard(detail::interceptor_state<T>& s) noexcept
            : state{s}
        {
            ++state.invoke_depth;
        }
        ~invocation_guard()
        {
            if (--state.invoke_depth == 0 && !state.active)
                state.request = {};
        }
        invocation_guard(const invocation_guard&) = delete;
        invocation_guard& operator=(const invocation_guard&) = delete;
        detail::interceptor_state<T>& state;
    };

public:
    using value_type = T;

    interceptor_handle<T>
        intercept(std::function<void(change_request<T>&)> request,
                  std::function<void()> invalidated = {})
    {
        if (interceptor_ && interceptor_->active)
            throw std::logic_error("property already has an interceptor");
        if (!request)
            throw std::invalid_argument(
                "interceptor callback must not be empty");
        interceptor_ = std::make_shared<detail::interceptor_state<T>>(
            detail::interceptor_state<T>{this, std::move(request),
                                         std::move(invalidated)});
        return interceptor_handle<T>{interceptor_};
    }

    property() = default;

    property(T value) noexcept : value_{std::move(value)} {}
    property(value_provider_ptr<T> provider) { assign(std::move(provider)); }

#ifdef CIRCLE_PROPERTY_NONCOPYABLE
    property(const property&) = delete;
    property& operator=(const property&) = delete;
#else
    property(const property& p) noexcept : value_{p.get()} {}
    property& operator=(const property& p)
    {
        *this = *p;
        return *this;
    }
#endif

    ~property()
    {
        invalidate_publications();
        invalidate_interception();
        before_destroyed_.emit(*this);
    }

    property(property&& other) noexcept
        : value_{std::move(other.value_)},
          value_changing_{std::move(other.value_changing_)},
          value_changed_{std::move(other.value_changed_)},
          moved_{std::move(other.moved_)},
          before_destroyed_{std::move(other.before_destroyed_)},
          value_changed_by_provider_{
              std::exchange(other.value_changed_by_provider_, false)}
    {
        other.invalidate_publications();
        interceptor_ = std::move(other.interceptor_);
        if (interceptor_)
            interceptor_->owner = this;
        provider_ = std::move(other.provider_);
        connect_provider();
        moved_.emit(*this);
    }

    property& operator=(property&& other) noexcept
    {
        if (this == &other)
            return *this;
        invalidate_publications();
        invalidate_interception();
        detach();
        value_ = std::move(other.value_);
        value_changing_ = std::move(other.value_changing_);
        value_changed_ = std::move(other.value_changed_);
        moved_ = std::move(other.moved_);
        before_destroyed_ = std::move(other.before_destroyed_);
        value_changed_by_provider_ =
            std::exchange(other.value_changed_by_provider_, false);

        other.invalidate_publications();
        interceptor_ = std::move(other.interceptor_);
        if (interceptor_)
            interceptor_->owner = this;
        provider_ = std::move(other.provider_);
        connect_provider();
        moved_.emit(*this);
        return *this;
    }

    property& operator=(const T& value)
    {
        detach();
        request_value(value, true);
        return *this;
    }

    property& operator=(T&& value)
    {
        detach();
        request_value(std::move(value), true);
        return *this;
    }

    property& operator=(value_provider_ptr<T> provider)
    {
        assign(std::move(provider));
        return *this;
    }

    bool assign(const T& value)
    {
        detach();
        return request_value(value, true);
    }

    bool assign(T&& value)
    {
        detach();
        return request_value(std::move(value), true);
    }

    bool assign(value_provider_ptr<T> provider)
    {
        detach();
        if (provider)
        {
            provider_ = std::move(provider);
            connect_provider();
            return request_value(provider_->get(), true);
        }
        return false;
    }

    bool detach()
    {
        if (provider_)
        {
            provider_.reset();
            return true;
        }
        return false;
    }

    constexpr const T& get() const noexcept { return value_; }
    constexpr const T& operator*() const noexcept { return get(); }
    constexpr operator const T&() const noexcept { return get(); }
    constexpr const T* operator->() const noexcept { return &get(); }

    const signal<property&>& value_changing() const { return value_changing_; }
    const signal<property&>& value_changed() const { return value_changed_; }
    const signal<property&>& moved() const { return moved_; }
    const signal<property&>& before_destroyed() const
    {
        return before_destroyed_;
    }

    template <typename F, typename... LArgs>
    connection connect(F&& f, LArgs&&... largs) const
    {
        auto s = [f = std::forward<F>(f),
                  largs...](const property& args) mutable {
            detail::invoke(f, largs..., args);
        };
        detail::invoke(s, *this);
        return value_changed_.connect(s);
    }

    template <typename F>
    void operator|=(F&& f) const
    {
        detail::invoke(f, *this);
        value_changed_ += std::forward<F>(f);
    }

    // Workaround for MSVC bug / comparing property<scoped enum>
    // https://developercommunity.visualstudio.com/t/scoped-enum-compared-against-a-class-wit/10061024
    friend constexpr bool operator==(const property& a, const property& b)
    {
        return a.get() == b.get();
    }

    friend constexpr bool operator==(const T& a, const property& b)
    {
        return a == b.get();
    }

    friend constexpr bool operator==(const property& a, const T& b)
    {
        return a.get() == b;
    }

private:
    void invalidate_publications() noexcept
    {
        while (publication_guards_)
        {
            publication_guards_->owner = nullptr;
            publication_guards_ = publication_guards_->previous;
        }
    }

    void invalidate_interception()
    {
        if (interceptor_)
        {
            interceptor_->owner = nullptr;
            interceptor_handle<T>{interceptor_}.reset();
        }
    }

    void connect_provider()
    {
        if (!provider_)
            return;
        provider_->set_updating_callback([this] { materialize(); });
        provider_->set_updated_callback([this] { on_provider_updated(); });
        provider_->set_before_invalid_callback([this] { detach(); });
    }

    template <typename U>
    bool request_value(U&& value, bool notify)
    {
        auto state = interceptor_;
        if (state && state->active)
        {
            const auto revision = ++state->revision;
            change_request<T> request{value};
            {
                invocation_guard guard{*state};
                state->request(request);
            }
            if (request.deferred() || state->owner != this || !state->active ||
                state->revision != revision)
                return false;
            return assign_impl(std::forward<U>(value), notify);
        }
        return assign_impl(std::forward<U>(value), notify);
    }

    void on_provider_updated()
    {
        if (std::exchange(value_changed_by_provider_, false))
        {
            value_changed_.emit(*this);
        }
    }

    bool materialize()
    {
        if (provider_)
        {
            return request_value(provider_->get(), false);
        }
        return false;
    }

    template <typename U>
    bool assign_impl(U&& value, bool notify)
    {
        if (!detail::eq(value, value_))
        {
            publication_guard guard{*this};
            value_ = std::forward<U>(value);
            if (!notify)
                value_changed_by_provider_ = true;
            value_changing_.emit(*this);
            if (notify && guard.owner)
            {
                value_changed_.emit(*this);
            }
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    std::shared_ptr<detail::interceptor_state<T>> interceptor_;
    publication_guard* publication_guards_{};
    T value_{};
    value_provider_ptr<T> provider_;
    signal<property&> value_changing_;
    signal<property&> value_changed_;
    signal<property&> moved_;
    signal<property&> before_destroyed_;

    bool value_changed_by_provider_{};
};

template <typename T>
std::optional<T> interceptor_handle<T>::current() const
{
    auto s = state_.lock();
    if (s && s->active && s->owner)
        return s->owner->get();
    return std::nullopt;
}

template <typename T>
bool interceptor_handle<T>::publish(const T& value) const
{
    auto s = state_.lock();
    return s && s->active && s->owner ? s->owner->assign_impl(value, true)
                                      : false;
}

template <typename T>
class property_ref
{
public:
    using value_type = T;

    property_ref(const property<T>& prop) noexcept
        : property_{&prop}, moved_connection_{connect_moved()}
    {
    }

    property_ref(const property_ref& other) noexcept
        : property_{other.property_}, moved_connection_{connect_moved()}
    {
    }
    property_ref& operator=(const property_ref& other) noexcept
    {
        property_ = other.property_;
        moved_connection_ = connect_moved();
        return *this;
    }

    property_ref(property_ref&& other) noexcept
        : property_{other.property_}, moved_connection_{connect_moved()}
    {
        other.moved_connection_.disconnect();
    }

    property_ref& operator=(property_ref&& other) noexcept
    {
        property_ = other.property_;
        moved_connection_ = connect_moved();
        other.moved_connection_.disconnect();
        return *this;
    }

    auto& value_changing()
    {
        assert(property_);
        return property_->value_changing();
    }

    auto& value_changed()
    {
        assert(property_);
        return property_->value_changed();
    }

    auto& before_destroyed()
    {
        assert(property_);
        return property_->before_destroyed();
    }

    const T& get() const { return *property_; }

    const T& operator*() const { return get(); }

    operator const T&() const { return get(); }

private:
    connection connect_moved() noexcept
    {
        return property_->moved().connect(
            [this](property<T>& p) noexcept { on_moved(p); });
    }

    void on_moved(property<T>& prop) noexcept { property_ = &prop; }

private:
    const property<T>* property_;
    scoped_connection moved_connection_;
};

} // namespace circle
