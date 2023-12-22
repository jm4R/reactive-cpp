#pragma once

#include <circle/reactive/signal.hpp>

#include <memory>

namespace circle {

namespace detail {

template <typename T>
struct ptr_data
{
    T obj_;
    signal<T&> before_destroyed_;

    template <typename... Args>
    ptr_data(Args&&... args) : obj_{std::forward<Args>(args)...}
    {
    }
};

} // namespace detail

template <typename T>
class ptr final
{
    template <typename T1>
    friend class weak_ptr;

    template <typename T1, typename... Args>
    ptr<T1> make_ptr(Args&&... args);

    std::unique_ptr<detail::ptr_data<T>> ptr_;

public:
    ptr() = default;
    ptr(std::unique_ptr<detail::ptr_data<T>> ptr) : ptr_{std::move(ptr)} {}

    ~ptr()
    {
        if (ptr_)
        {
            ptr_->before_destroyed_(*get());
        }
    }

    ptr(const ptr& other) = delete;
    ptr& operator=(const ptr& other) = delete;

    ptr(ptr&& other) noexcept = default;
    ptr& operator=(ptr&& other) noexcept = default;

    signal<T&>& before_destroyed()
    {
        assert(ptr_);
        return ptr_->before_destroyed_;
    }

    void reset() noexcept
    {
        if (ptr_)
        {
            ptr_->before_destroyed_(*get());
            ptr_.reset();
        }
    };

    void operator=(std::nullptr_t) noexcept
    {
        reset();
    }

    void swap(ptr& other) noexcept
    {
        ptr_.swap(other);
    };

    const T* get() const noexcept { return ptr_ ? &ptr_->obj_ : nullptr; }
    T* get() noexcept { return ptr_ ? &ptr_->obj_ : nullptr; }

    explicit operator bool() const noexcept { return !!ptr_; }

    const T& operator*() const noexcept
    {
        assert(ptr_);
        return *get();
    }
    T& operator*() noexcept
    {
        assert(ptr_);
        return *get();
    }

    const T* operator->() const noexcept
    {
        assert(ptr_);
        return get();
    }
    T* operator->() noexcept
    {
        assert(ptr_);
        return get();
    }

    bool operator==(T* ptr) const noexcept { return ptr == ptr_; }
    bool operator!=(T* ptr) const noexcept { return ptr != ptr_; }
    bool operator==(std::nullptr_t) const noexcept { return !ptr_; }
    bool operator!=(std::nullptr_t) const noexcept { return !!ptr_; }

    template <typename T2>
    bool operator==(const ptr<T2>& other) const noexcept
    {
        return ptr_ == other.ptr_;
    }
    template <typename T2>
    bool operator!=(const ptr<T2>& other) const noexcept
    {
        return ptr_ != other.ptr_;
    }
};

template <typename T, typename... Args>
ptr<T> make_ptr(Args&&... args)
{
    return {std::make_unique<detail::ptr_data<T>>(std::forward<Args>(args)...)};
}

template <typename T>
class weak_ptr
{
    detail::ptr_data<T>* src_;
    scoped_connection destroyed_connection_;

public:
    weak_ptr() = default;
    ~weak_ptr() = default;

    weak_ptr(const ptr<T>& src) noexcept
        : src_{src ? src.ptr_.get() : nullptr},
          destroyed_connection_{connect_destroyed()}
    {
    }

    weak_ptr(const weak_ptr& other)
        : src_{other.src_}, destroyed_connection_{connect_destroyed()}
    {
    }
    weak_ptr& operator=(const weak_ptr& other)
    {
        if (src_ != other.src_)
        {
            src_ = other.src_;
            destroyed_connection_ = connect_destroyed();
        }
    }

    weak_ptr(weak_ptr&& other) noexcept = default;
    weak_ptr& operator=(weak_ptr&& other) noexcept = default;

    signal<T&>& before_destroyed()
    {
        assert(src_);
        return src_->before_destroyed_;
    }

    explicit operator bool() const noexcept { return !!src_(); }
    bool operator==(T* ptr) const noexcept { return ptr == src_; }
    bool operator!=(T* ptr) const noexcept { return ptr != src_; }
    bool operator==(std::nullptr_t) const noexcept { return !src_; }
    bool operator!=(std::nullptr_t) const noexcept { return !!src_; }

    template <typename T2>
    bool operator==(const ptr<T2>& other) const noexcept
    {
        return (!src_ && !other.src_) ||
               (src_ && other.src_ && *src_ == *other.src_);
    }
    template <typename T2>
    bool operator!=(const ptr<T2>& other) const noexcept
    {
        return !(*this == other);
    }

    const T* operator->() const noexcept
    {
        assert(src_);
        return get();
    }

    T* operator->() noexcept
    {
        assert(src_);
        return get();
    }

    const T& operator*() const noexcept
    {
        assert(src_);
        return *get();
    }

    T& operator*() noexcept
    {
        assert(src_);
        return *get();
    }

    const T* get() const noexcept
    {
        return src_ ? &src_->obj_ : nullptr;
    }

    T* get() noexcept
    {
        return src_ ? &src_->obj_ : nullptr;
    }

private:
    connection connect_destroyed() noexcept
    {
        if (!src_)
            return {};
        return src_->before_destroyed_.connect(
            [this](T& p) noexcept { on_destroyed(p); });
    }

    void on_destroyed(T& prop) noexcept
    {
        src_ = nullptr;
    }
};

} // namespace circle