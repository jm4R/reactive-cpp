#pragma once

#include <circle/bind/signal.hpp>

#include <memory>

namespace circle {

template <typename T>
class value_provider
{
public:
    virtual ~value_provider() {};
    virtual signal<>& updated() = 0;
    virtual const T& get() = 0;
};

template <typename T>
using value_provider_ptr = std::unique_ptr<value_provider<T>>;

template <typename T>
class property
{
public:
    property() : value_{} {}

    property(T value) : value_{std::move(value)} {}

    property(const property&) = delete;
    property& operator=(const property&) = delete;

    property(property&&) = default;
    property& operator=(property&&) = default;

    property& operator=(T value)
    {
        assign(std::move(value));
        return *this;
    }

    property& operator=(value_provider_ptr<T> provider)
    {
        detach();
        if (provider)
        {
            provider_ = std::move(provider);
            provider_observer_ = provider_->updated().connect([this] {
                dirty_ = true;
                value_changed_.emit(*this);
            });
            dirty_ = true;
            if (materialize())
                value_changed_.emit(*this);
        }
        return *this;
    }

    bool assign(T value)
    {
        if (value != value_)
        {
            using std::swap;
            swap(value, value_);
            value_changed_.emit(*this);
            return true;
        }
        else
        {
            return false;
        }
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

    operator const T&() const
    {
        materialize();
        return value_;
    }

    signal<property&>& value_changed() { return value_changed_; }

private:
    bool materialize() const
    {
        if (provider_ && dirty_)
        {
            dirty_ = false;
            auto new_value = provider_->get();
            if (new_value != value_)
            {
                const_cast<T&>(value_) = std::move(new_value);
                return true;
            }
        }
        return false;
    }

private:
    T value_;
    mutable std::unique_ptr<value_provider<T>> provider_;
    mutable bool dirty_{};
    connection provider_observer_;
    signal<property&> value_changed_;
};

} // namespace circle