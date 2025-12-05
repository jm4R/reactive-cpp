#pragma once

#include <circle/reactive/signal.hpp>

#include <memory>

namespace circle {

namespace detail {

class tracking_from_this_tag
{
};

template <typename T>
concept tracking_from_this_enabled =
    std::is_base_of<tracking_from_this_tag, T>::value;

template <typename T>
struct obj_with_signal
{
    T obj_;
    signal<> before_destroyed_;

    template <typename... Args>
    obj_with_signal(Args&&... args) : obj_{std::forward<Args>(args)...}
    {
    }
};

template <tracking_from_this_enabled T>
struct obj_with_signal<T>
{
    T obj_;
    signal<>& before_destroyed_; // in this specialization signal must already
                                 // be constructed on T's constructor to be able
                                 // to use tracking_from_this from inside. We
                                 // only keep reference here.

    template <typename... Args>
    obj_with_signal(Args&&... args)
        : obj_{std::forward<Args>(args)...},
          before_destroyed_{obj_.before_destroyed_}
    {
    }
};

template <typename T>
struct ptr_data
{
    std::shared_ptr<T> data_;
    signal<>* before_destroyed_{};

    operator bool() const { return !!data_; }
};

template <typename T, typename... Args>
ptr_data<T> make_ptr_data(Args&&... args)
{
    auto ows =
        std::make_shared<obj_with_signal<T>>(std::forward<Args>(args)...);
    auto obj_ptr = std::shared_ptr<T>{ows, &ows->obj_}; // aliasing constructor
    return {std::move(obj_ptr), &ows->before_destroyed_};
}

} // namespace detail

template <typename T>
class ptr final
{
    template <typename T1>
    friend class tracking_ptr;

    template <typename T1, typename... Args>
    friend ptr<T1> make_ptr(Args&&... args);

    template <typename U>
    friend class ptr;

    detail::ptr_data<T> data_;

public:
    using value_type = T;

    ptr() = default;
    ptr(detail::ptr_data<T> ptr) : data_{std::move(ptr)} {}

    ~ptr() { reset(); }

    ptr(const ptr& other) = delete;
    ptr& operator=(const ptr& other) = delete;

    ptr(ptr&& other) noexcept = default;
    ptr& operator=(ptr&& other) noexcept = default;

    template <typename Derived,
              std::enable_if_t<std::is_base_of_v<T, Derived>, int> = 0>
    ptr(ptr<Derived>&& other) noexcept
        : data_{std::move(other.data_.data_), other.data_.before_destroyed_}
    {
    }

    template <typename Derived,
              std::enable_if_t<std::is_base_of_v<T, Derived>, int> = 0>
    ptr& operator=(ptr<Derived>&& other) noexcept
    {
        if (this != reinterpret_cast<ptr*>(&other))
        {
            reset();
            if (other.data_)
            {
                data_ = std::move(other.data_);
                other.data_.data_.reset();
            }
        }
        return *this;
    }

    signal<>& before_destroyed()
    {
        assert(data_);
        return *data_.before_destroyed_;
    }

    void reset() noexcept
    {
        if (data_)
        {
            before_destroyed()();
            data_.data_.reset();
            data_.before_destroyed_ = nullptr;
        }
    };

    void operator=(std::nullptr_t) noexcept { reset(); }

    T* get() const noexcept { return data_ ? data_.data_.get() : nullptr; }

    explicit operator bool() const noexcept { return !!data_; }

    const T& operator*() const noexcept
    {
        assert(data_);
        return *get();
    }
    T& operator*() noexcept
    {
        assert(data_);
        return *get();
    }

    const T* operator->() const noexcept
    {
        assert(data_);
        return get();
    }
    T* operator->() noexcept
    {
        assert(data_);
        return get();
    }

    bool operator==(T* ptr) const noexcept { return ptr == data_.data_.get(); }
    bool operator!=(T* ptr) const noexcept { return ptr != data_.data_.get(); }
    bool operator==(std::nullptr_t) const noexcept { return !data_; }
    bool operator!=(std::nullptr_t) const noexcept { return !!data_; }

    template <typename T2>
    bool operator==(const ptr<T2>& other) const noexcept
    {
        return data_.data_ == other.data_.data_;
    }
    template <typename T2>
    bool operator!=(const ptr<T2>& other) const noexcept
    {
        return data_.data_ != other.data_.data_;
    }
};

template <typename T>
class tracking_ptr
{
    T* ptr_{};
    signal<>* before_destroyed_{};
    scoped_connection destroyed_connection_;

    template <typename T2>
    friend class tracking_ptr;

public:
    using value_type = T;

    tracking_ptr() = default;
    ~tracking_ptr() = default;

    tracking_ptr(std::nullptr_t) {}

    tracking_ptr(const ptr<T>& src) noexcept
        : ptr_{src.get()},
          before_destroyed_{src ? src.data_.before_destroyed_ : nullptr},
          destroyed_connection_{connect_destroyed()}
    {
    }

    template <typename Y>
    tracking_ptr(const ptr<Y>& src) noexcept
        : ptr_{src.get()},
          before_destroyed_{src ? src.data_.before_destroyed_ : nullptr},
          destroyed_connection_{connect_destroyed()}
    {
    }

    tracking_ptr(const tracking_ptr& other)
        : ptr_{other.ptr_},
          before_destroyed_{other.before_destroyed_},
          destroyed_connection_{connect_destroyed()}
    {
    }
    tracking_ptr& operator=(const tracking_ptr& other)
    {
        if (ptr_ != other.ptr_)
        {
            ptr_ = other.ptr_;
            before_destroyed_ = other.before_destroyed_;
            destroyed_connection_ = connect_destroyed();
        }
        return *this;
    }

    template <typename Y>
    tracking_ptr(const tracking_ptr<Y>& other)
        : ptr_{other.ptr_},
          before_destroyed_{other.before_destroyed_},
          destroyed_connection_{connect_destroyed()}
    {
    }
    template <typename Y>
    tracking_ptr& operator=(const tracking_ptr<Y>& other)
    {
        if (ptr_ != other.ptr_)
        {
            ptr_ = other.ptr_;
            before_destroyed_ = other.before_destroyed_;
            destroyed_connection_ = connect_destroyed();
        }
        return *this;
    }

    tracking_ptr(tracking_ptr&& other) noexcept
        : ptr_{other.ptr_},
          before_destroyed_{other.before_destroyed_},
          destroyed_connection_{connect_destroyed()}
    {
        other.ptr_ = nullptr;
        other.before_destroyed_ = nullptr;
        other.destroyed_connection_.disconnect();
    }
    tracking_ptr& operator=(tracking_ptr&& other) noexcept
    {
        if (ptr_ != other.ptr_)
        {
            ptr_ = other.ptr_;
            before_destroyed_ = other.before_destroyed_;
            destroyed_connection_ = connect_destroyed();
            other.ptr_ = nullptr;
            other.before_destroyed_ = nullptr;
            other.destroyed_connection_.disconnect();
        }
        return *this;
    }

    template <typename Y>
    tracking_ptr(tracking_ptr<Y>&& other) noexcept // 10b
        : ptr_{other.ptr_},
          before_destroyed_{other.before_destroyed_},
          destroyed_connection_{connect_destroyed()}
    {
        other.ptr_ = nullptr;
        other.before_destroyed_ = nullptr;
        other.destroyed_connection_.disconnect();
    }
    template <typename Y>
    tracking_ptr& operator=(tracking_ptr<Y>&& other) noexcept
    {
        if (ptr_ != other.ptr_)
        {
            ptr_ = other.ptr_;
            before_destroyed_ = other.before_destroyed_;
            destroyed_connection_ = connect_destroyed();
            other.ptr_ = nullptr;
            other.before_destroyed_ = nullptr;
            other.destroyed_connection_.disconnect();
        }
        return *this;
    }

private:
    template <class T2, class U>
    friend tracking_ptr<T2>
        static_pointer_cast(const tracking_ptr<U>& r) noexcept;
    template <typename T2>
    friend class enable_tracking_from_this;

    explicit tracking_ptr(T* ptr, signal<>* before_destroyed)
        : ptr_{ptr},
          before_destroyed_{before_destroyed},
          destroyed_connection_{connect_destroyed()}
    {
    }

public:
    signal<>& before_destroyed()
    {
        assert(before_destroyed_);
        return *before_destroyed_;
    }

    explicit operator bool() const noexcept { return !!ptr_; }
    bool operator==(T* ptr) const noexcept { return ptr == ptr_; }
    bool operator!=(T* ptr) const noexcept { return ptr != ptr_; }
    bool operator==(std::nullptr_t) const noexcept { return !ptr_; }
    bool operator!=(std::nullptr_t) const noexcept { return !!ptr_; }

    template <typename T2>
    bool operator==(const ptr<T2>& other) const noexcept
    {
        return ptr_ == other.get();
    }

    template <typename T2>
    bool operator==(const tracking_ptr<T2>& other) const noexcept
    {
        return ptr_ == other.ptr_;
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

    const T* get() const noexcept { return ptr_; }

    T* get() noexcept { return ptr_; }

private:
    connection connect_destroyed() noexcept
    {
        if (!ptr_)
            return {};
        return before_destroyed_->connect(
            [this]() noexcept { on_destroyed(); });
    }

    void on_destroyed() noexcept { ptr_ = nullptr; }
};

template <class T, class U>
tracking_ptr<T> static_pointer_cast(const tracking_ptr<U>& r) noexcept
{
    return tracking_ptr<T>{static_cast<T*>(r.ptr_), r.before_destroyed_};
}

template <typename T>
class enable_tracking_from_this : public detail::tracking_from_this_tag
{
public:
    template <typename T2 = T>
    tracking_ptr<T2> tracking_from_this()
    {
        // NOTE: calling this function from object created without make_ptr
        // usage is not currently verified and is treated as UB. This is
        // different compared to
        // std::enable_shared_from_this::shared_from_this() behavior!
        return tracking_ptr<T2>{static_cast<T2*>(this), &before_destroyed_};
    }

    template <typename T2>
    static tracking_ptr<T2> tracking_from(T2* value)
    {
        static_assert(detail::tracking_from_this_enabled<T2>);
        return tracking_ptr<T2>{static_cast<T2*>(value),
                                &value->before_destroyed_};
    }

private:
    template <typename T2>
    friend struct detail::obj_with_signal;

    signal<> before_destroyed_{};
};

template <typename T, typename... Args>
ptr<T> make_ptr(Args&&... args)
{
    return ptr<T>{detail::make_ptr_data<T>(std::forward<Args>(args)...)};
}

} // namespace circle