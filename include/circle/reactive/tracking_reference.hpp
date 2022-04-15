#pragma once

#include <circle/reactive/signal.hpp>

namespace circle {

template <typename T>
class enable_tracking_reference
{
public:
    enable_tracking_reference() = default;

    enable_tracking_reference(const enable_tracking_reference&) = delete;
    enable_tracking_reference&
        operator=(const enable_tracking_reference&) = delete;

    enable_tracking_reference(enable_tracking_reference&& other) = default;
    enable_tracking_reference&
        operator=(enable_tracking_reference&& other) = default;

    ~enable_tracking_reference() = default;

    signal<T&>& moved() { return moved_; }
    signal<T&>& before_destroyed() { return before_destroyed_; }

protected:
    void call_moved() { moved_.emit(static_cast<T&>(*this)); }
    void call_before_destroyed()
    {
        before_destroyed_.emit(static_cast<T&>(*this));
    }

private:
    signal<T&> moved_;
    signal<T&> before_destroyed_;
};

template <typename T>
class tracking_reference
{
public:
    tracking_reference(T& src) noexcept
        : src_{&src},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
    }

    tracking_reference(const tracking_reference& other) noexcept
        : src_{other.src_},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
    }
    tracking_reference& operator=(const tracking_reference& other) noexcept
    {
        src_ = other.property_;
        moved_connection_ = connect_moved();
        destroyed_connection_ = connect_destroyed();
    }

    tracking_reference(tracking_reference&& other) noexcept
        : src_{other.src_},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
        other.moved_connection_.disconnect();
        other.destroyed_connection_.disconnect();
    }
    tracking_reference& operator=(tracking_reference&& other) noexcept
    {
        src_ = other.src_;
        moved_connection_ = connect_moved();
        destroyed_connection_ = connect_destroyed();
        other.moved_connection_.disconnect();
        other.destroyed_connection_.disconnect();
        return *this;
    }

    bool is_dangling() const noexcept { return !src_; }

    explicit operator bool() const noexcept { return !is_dangling(); }

    const T* operator->() const noexcept
    {
        assert(!is_dangling());
        return src_;
    }

    T* operator->() noexcept
    {
        assert(!is_dangling());
        return src_;
    }

    const T& operator*() const noexcept
    {
        assert(!is_dangling());
        return *src_;
    }

    T& operator*() noexcept
    {
        assert(!is_dangling());
        return *src_;
    }

    const T* get() const noexcept { return src_; }

    T* get() noexcept { return src_; }

private:
    connection connect_moved() noexcept
    {
        return src_->moved().connect([this](T& p) noexcept { on_moved(p); });
    }

    void on_moved(T& prop) noexcept { src_ = &prop; }

    connection connect_destroyed() noexcept
    {
        return src_->before_destroyed().connect(
            [this](T& p) noexcept { on_destroyed(p); });
    }

    void on_destroyed(T& prop) noexcept { src_ = nullptr; }

private:
    T* src_;
    scoped_connection moved_connection_;
    scoped_connection destroyed_connection_;
};

} // namespace circle