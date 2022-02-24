#pragma once

#include <circle/reactive/signal.hpp>

namespace circle {

template <typename T>
class enable_ref
{
public:
    enable_ref() = default;

    enable_ref(const enable_ref&) = delete;
    enable_ref& operator=(const enable_ref&) = delete;

    enable_ref(enable_ref&& other) = default;
    enable_ref& operator=(enable_ref&& other) = default;

    ~enable_ref() = default;

    signal<T&>& moved() { return moved_; }
    signal<T&>& before_destroyed() { return before_destroyed_; }

public:
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
class reference
{
public:
    reference(T& src) noexcept
        : src_{&src},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
    }

    reference(const reference& other) noexcept
        : src_{other.src_},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
    }
    reference& operator=(const reference& other) noexcept
    {
        src_ = other.property_;
        moved_connection_ = connect_moved();
        destroyed_connection_ = connect_destroyed();
    }

    reference(reference&& other) noexcept
        : src_{other.src_},
          moved_connection_{connect_moved()},
          destroyed_connection_{connect_destroyed()}
    {
        other.moved_connection_.disconnect();
        other.destroyed_connection_.disconnect();
    }
    reference& operator=(reference&& other) noexcept
    {
        src_ = other.src_;
        moved_connection_ = connect_moved();
        destroyed_connection_ = connect_destroyed();
        other.moved_connection_.disconnect();
        other.destroyed_connection_.disconnect();
        return *this;
    }

    bool is_dangling() const noexcept
    {
        return is_dangling_;
    }

    const T* operator->() const noexcept
    {
        assert(!is_dangling_);
        return src_;
    }

    T* operator->() noexcept
    {
        assert(!is_dangling_);
        return src_;
    }

    const T& operator*() const noexcept
    {
        assert(!is_dangling_);
        return *src_;
    }

    T& operator*() noexcept
    {
        assert(!is_dangling_);
        return *src_;
    }

    const T* get() const noexcept
    {
        return is_dangling_ ? static_cast<T*>(nullptr) : src_;
    }

    T* get() noexcept
    {
        return is_dangling_ ? static_cast<T*>(nullptr) : src_;
    }

private:
    connection connect_moved() noexcept
    {
        return src_->moved().connect(
            [this](T& p) noexcept { on_moved(p); });
    }

    void on_moved(T& prop) noexcept { src_ = &prop; }

    connection connect_destroyed() noexcept
    {
        return src_->before_destroyed().connect(
            [this](T& p) noexcept { on_destroyed(p); });
    }

    void on_destroyed(T& prop) noexcept { src_ = &prop; }

private:
    T* src_;
    scoped_connection moved_connection_;
    scoped_connection destroyed_connection_;
    bool is_dangling_{};
};

} // namespace circle