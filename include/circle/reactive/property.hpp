#pragma once

#include <circle/reactive/signal.hpp>

#include <functional>
#include <memory>

namespace circle {

namespace detail {
template <typename T, typename = bool>
struct is_equality_comparable : std::false_type
{
};

template <typename T>
struct is_equality_comparable<
    T, typename std::enable_if_t<true, decltype(std::declval<const T&>() ==
                                                std::declval<const T&>())>>
    : std::true_type
{
};

template <typename T>
constexpr bool eq(const T& v1, const T& v2)
{
    if constexpr (is_equality_comparable<T>::value)
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
class property
{
public:
    using value_type = T;

    property() = default;

    property(T value) noexcept : value_{std::move(value)} {}
    property(value_provider_ptr<T> provider) { assign(std::move(provider)); }

    property(const property&) = delete;
    property& operator=(const property&) = delete;

    ~property() { before_destroyed_.emit(*this); }

    property(property&& other) noexcept
        : value_{std::move(other.value_)},
          value_changing_{std::move(other.value_changing_)},
          value_changed_{std::move(other.value_changed_)},
          moved_{std::move(other.moved_)},
          before_destroyed_{std::move(other.before_destroyed_)}
    {
        other.provider_observer_.disconnect();
        assign(std::move(other.provider_));
        moved_.emit(*this);
    }

    property& operator=(property&& other) noexcept
    {
        value_ = std::move(other.value_);
        value_changing_ = std::move(other.value_changing_);
        value_changed_ = std::move(other.value_changed_);
        moved_ = std::move(other.moved_);
        before_destroyed_ = std::move(other.before_destroyed_);

        other.provider_observer_.disconnect();
        assign(std::move(other.provider_));
        moved_.emit(*this);
        return *this;
    }

    property& operator=(const T& value)
    {
        detach();
        assign_impl(value, true);
        return *this;
    }

    property& operator=(T&& value)
    {
        detach();
        assign_impl(std::move(value), true);
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
        return assign_impl(value, true);
    }

    bool assign(T&& value)
    {
        detach();
        return assign_impl(std::move(value), true);
    }

    bool assign(value_provider_ptr<T> provider)
    {
        detach();
        if (provider)
        {
            provider_ = std::move(provider);
            provider_->set_updating_callback([this] { materialize(); });
            provider_->set_updated_callback([this] { on_provider_updated(); });
            provider_->set_before_invalid_callback([this] { detach(); });
            return assign_impl(provider_->get(), true);
        }
        return false;
    }

    bool detach()
    {
        if (provider_)
        {
            provider_observer_.disconnect();
            provider_.reset();
            return true;
        }
        return false;
    }

    const T& get() const
    {
        return value_;
    }

    const T& operator*() const { return get(); }

    operator const T&() const { return get(); }

    const T* operator->() const { return &get(); }

    signal<property&>& value_changing() const { return value_changing_; }
    signal<property&>& value_changed() const { return value_changed_; }
    signal<property&>& moved() const { return moved_; }
    signal<property&>& before_destroyed() const { return before_destroyed_; }

    template <typename F>
    void operator|=(F&& f) const
    {
        detail::invoke(f, *this);
        value_changed_ += std::forward<F>(f);
    }

private:
    void on_provider_updated()
    {
        if (value_changed_by_provider_)
        {
            value_changed_.emit(*this);
            value_changed_by_provider_ = false;
        }
    }

    bool materialize()
    {
        if (provider_)
        {
            if (assign_impl(provider_->get(), false))
            {
                value_changed_by_provider_ = true;
                return true;
            }
        }
        return false;
    }

    template <typename U>
    bool assign_impl(U&& value, bool notify)
    {
        if (!detail::eq(value, value_))
        {
            value_ = std::forward<U>(value);
            value_changing_.emit(*this);
            if (notify)
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
    T value_{};
    value_provider_ptr<T> provider_;
    scoped_connection provider_observer_;
    mutable signal<property&> value_changing_;
    mutable signal<property&> value_changed_;
    mutable signal<property&> moved_;
    mutable signal<property&> before_destroyed_;

    bool value_changed_by_provider_{};
};

template <typename T>
class property_ref // read-only for now
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