#pragma once

#include <circle/reactive/property.hpp>

#include <array>

namespace circle {

namespace detail {

template <typename T>
struct transaction_entry
{
    signal_blocker changing_blocker;
    signal_blocker changed_blocker;
    property<T>& reference;
    T start_value;
};

} // namespace detail

template <typename... Args>
class transaction
{
public:
    transaction(Args&... props) : entries_(make_entry(props)...) {}

    transaction(const transaction&) = delete;
    transaction& operator=(const transaction&) = delete;
    transaction(transaction&& other) = delete;
    transaction& operator=(transaction&&) = delete;

    void commit()
    {
        std::apply([&](auto&... prop) { (emit_chaning(prop), ...); }, entries_);
        std::apply([&](auto&... prop) { (emit_changed(prop), ...); }, entries_);
    }

private:
    template <typename T>
    auto make_entry(property<T>& p)
    {
        return detail::transaction_entry<T>{
            .changing_blocker = signal_blocker{p.value_changing()},
            .changed_blocker = {p.value_changed()},
            .reference = p,
            .start_value = p.get(),
        };
    }

    template <typename T>
    void emit_chaning(detail::transaction_entry<T>& p)
    {
        p.changing_blocker.dismiss();
        if (!detail::eq(p.reference.get(), p.start_value))
        {
            p.reference.value_changing().emit(p.reference);
        }
    }
    template <typename T>
    void emit_changed(detail::transaction_entry<T>& p)
    {
        p.changed_blocker.dismiss();
        if (!detail::eq(p.reference.get(), p.start_value))
        {
            p.reference.value_changed().emit(p.reference);
        }
    }

private:
    constexpr static std::size_t N = sizeof...(Args);
    static_assert(N != 0, "Can't create empty transaction");
    std::tuple<detail::transaction_entry<typename Args::value_type>...>
        entries_;
};

} // namespace circle