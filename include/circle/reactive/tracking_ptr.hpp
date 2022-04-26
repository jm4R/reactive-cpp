#pragma once

#include <circle/reactive/signal.hpp>

#include <cstddef>

namespace circle {

template <typename T>
class enable_tracking_ptr
{
public:
    enable_tracking_ptr() = default;

    enable_tracking_ptr(const enable_tracking_ptr&) = delete;
    enable_tracking_ptr& operator=(const enable_tracking_ptr&) = delete;

    enable_tracking_ptr(enable_tracking_ptr&& other) = default;
    enable_tracking_ptr& operator=(enable_tracking_ptr&& other) = default;

    ~enable_tracking_ptr() = default;

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
class tracking_ptr
{
public:
    tracking_ptr() = default;

    tracking_ptr(T* src) noexcept
        : src_{src},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
    }

    tracking_ptr(const tracking_ptr& other) noexcept
        : src_{other.src_},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
    }
    tracking_ptr& operator=(const tracking_ptr& other) noexcept
    {
        src_ = other.property_;
        moved_connection_ = connect_moved();
        destroyed_connection_ = connect_destroyed();
    }

    tracking_ptr(tracking_ptr&& other) noexcept
        : src_{other.src_},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
        other.moved_connection_.disconnect();
        other.destroyed_connection_.disconnect();
    }
    tracking_ptr& operator=(tracking_ptr&& other) noexcept
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
    bool operator==(T* ptr) const noexcept { return ptr == src_; }
    bool operator!=(T* ptr) const noexcept { return ptr != src_; }
    bool operator==(std::nullptr_t) const noexcept { return is_dangling(); }
    bool operator!=(std::nullptr_t) const noexcept { return !is_dangling(); }

    template <typename T2>
    bool operator==(const tracking_ptr<T2>& other) const noexcept
    {
        return src_ == other.src_;
    }
    template <typename T2>
    bool operator!=(const tracking_ptr<T2>& other) const noexcept
    {
        return src_ != other.src_;
    }

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
        if (!src_)
            return {};
        return src_->moved().connect([this](T& p) noexcept { on_moved(p); });
    }

    void on_moved(T& prop) noexcept { src_ = &prop; }

    connection connect_destroyed() noexcept
    {
        if (!src_)
            return {};
        return src_->before_destroyed().connect(
            [this](T& p) noexcept { on_destroyed(p); });
    }

    void on_destroyed(T& prop) noexcept { src_ = nullptr; }

private:
    T* src_{};
    scoped_connection moved_connection_;
    scoped_connection destroyed_connection_;
};

} // namespace circle