#pragma once

#include <circle/reactive/signal.hpp>
#include <circle/reactive/utils.hpp>

#include <memory>

namespace circle {

template <typename T>
class value_provider
{
public:
    virtual ~value_provider() {}
    virtual signal<>& updated() = 0;
    virtual T get() = 0;
};

template <typename T>
using value_provider_ptr = std::unique_ptr<value_provider<T>>;

template <typename T>
class property
{
public:
    property() = default;

    property(T value) : value_{std::move(value)} {}
    property(value_provider_ptr<T> provider) { assign(std::move(provider)); }

    property(const property&) = delete;
    property& operator=(const property&) = delete;

    property(property&& other)
        : value_{std::move(other.value_)},
          // dirty_{other.dirty_},
          value_changed_{std::move(other.value_changed_)},
          moved_{std::move(other.moved_)}
    {
        other.provider_observer_.disconnect();
        assign(std::move(other.provider_));
        moved_.emit(*this);
    }

    property& operator=(property&& other)
    {
        value_ = std::move(other.value_);
        // dirty_ = other.dirty_;
        value_changed_ = std::move(other.value_changed_);
        moved_ = std::move(other.moved_);

        other.provider_observer_.disconnect();
        assign(std::move(other.provider_));
        moved_.emit(*this);
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
        if (value != value_)
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
            provider_observer_ = provider_->updated().connect([this] {
                dirty_ = true;
                value_changed_.emit(*this);
            });
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

    const T& operator*() const
    {
        return get();
    }

    operator const T&() const
    {
        return get();
    }

    signal<property&>& value_changed() { return value_changed_; }
    signal<property&>& moved() { return moved_; }

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
    T value_{};
    mutable value_provider_ptr<T> provider_;
    mutable bool dirty_{};
    scoped_connection provider_observer_;
    signal<property&> value_changed_;
    signal<property&> moved_;
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
class property_ref // read-only for now
{
public:
    property_ref(property<T>& prop)
        : property_{&prop}, moved_connection_{connect_moved()}
    {
    }

    property_ref(const property_ref& other)
        : property_{other.property_}, moved_connection_{connect_moved()}
    {
    }
    property_ref& operator=(const property_ref& other)
    {
        property_ = other.property_;
        moved_connection_ = connect_moved();
    }

    property_ref(property_ref&& other)
        : property_{other.property_}, moved_connection_{connect_moved()}
    {
        other.moved_connection_.disconnect();
    }

    property_ref& operator=(property_ref&& other)
    {
        property_ = other.property_;
        moved_connection_ = connect_moved();
        other.moved_connection_.disconnect();
        return *this;
    }

    operator const T&() const { return *property_; }

private:
    connection connect_moved()
    {
        return property_->moved().connect(
            [this](property<T>& p) { on_moved(p); });
    }

    void on_moved(property<T>& prop) { property_ = &prop; }

private:
    property<T>* property_;
    scoped_connection moved_connection_;
};

} // namespace circle