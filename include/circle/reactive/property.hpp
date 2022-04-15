#pragma once

#include <circle/reactive/signal.hpp>
#include <circle/reactive/tracking_reference.hpp>

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
    virtual void set_updated_callback(std::function<void()> clb) = 0;
    virtual void set_before_invalid_callback(std::function<void()> clb) = 0;
    virtual T get() = 0;
};

template <typename T>
using value_provider_ptr = std::unique_ptr<value_provider<T>>;

template <typename T>
class property : public enable_tracking_reference<property<T>>
{
public:
    property() = default;

    property(T value) noexcept : value_{std::move(value)} {}
    property(value_provider_ptr<T> provider) { assign(std::move(provider)); }

    property(const property&) = delete;
    property& operator=(const property&) = delete;

    ~property() { this->call_before_destroyed(); }

    property(property&& other) noexcept
        : enable_tracking_reference<property<T>>{std::move(other)},
          value_{std::move(other.value_)},
          // dirty_{other.dirty_},
          value_changed_{std::move(other.value_changed_)}
    {
        other.provider_observer_.disconnect();
        assign(std::move(other.provider_));
        this->call_moved();
    }

    property& operator=(property&& other) noexcept
    {
        enable_tracking_reference<property<T>>::operator=(std::move(other));
        value_ = std::move(other.value_);
        // dirty_ = other.dirty_;
        value_changed_ = std::move(other.value_changed_);

        other.provider_observer_.disconnect();
        assign(std::move(other.provider_));
        this->call_moved();
        return *this;
    }

    property& operator=(T value)
    {
        assign(std::move(value));
        return *this;
    }

    property& operator=(value_provider_ptr<T> provider)
    {
        assign(std::move(provider));
        return *this;
    }

    bool assign(T value)
    {
        if (!detail::eq(value, value_))
        {
            value_ = std::move(value);
            value_changed_.emit(*this);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool assign(value_provider_ptr<T> provider)
    {
        detach();
        if (provider)
        {
            provider_ = std::move(provider);
            provider_->set_updated_callback([this] {
                dirty_ = true;
                value_changed_.emit(*this);
            });
            provider_->set_before_invalid_callback([this] { detach(); });
            dirty_ = true;
            if (materialize())
            {
                value_changed_.emit(*this);
                return true;
            }
        }
        return false;
    }

    bool detach()
    {
        materialize();
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
        materialize();
        return value_;
    }

    const T& operator*() const { return get(); }

    operator const T&() const { return get(); }

    signal<property&>& value_changed() { return value_changed_; }

private:
    bool materialize() const
    {
        if (provider_ && dirty_)
        {
            dirty_ = false;
            auto new_value = provider_->get();
            if (!detail::eq(new_value, value_))
            {
                const_cast<T&>(value_) = std::move(new_value);
                return true;
            }
        }
        return false;
    }

private:
    T value_{};
    mutable value_provider_ptr<T> provider_;
    mutable bool dirty_{};
    scoped_connection provider_observer_;
    signal<property&> value_changed_;
};

template <typename T>
struct is_property : std::false_type
{
};

template <typename T>
struct is_property<property<T>> : std::true_type
{
};

template <typename T>
class property_ref : public tracking_reference<property<T>>
{
    // Aliases doesn't honor CTAD in C++17, so just derive from
    // tracking_reference
public:
    property_ref(property<T>& src) noexcept
        : tracking_reference<property<T>>{src}
    {
    }
};

} // namespace circle