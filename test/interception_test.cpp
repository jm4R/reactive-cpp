#include <catch2/catch_all.hpp>
#include <circle/reactive/bind.hpp>
#include <circle/reactive/property.hpp>
#include <circle/reactive/transaction.hpp>
#include <memory>
#include <optional>
#include <vector>
using namespace circle;

TEST_CASE("interceptor invokes the same mutable callback without copying")
{
    property<int> target{0};
    int observed = 0;
    auto channel = target.intercept(
        [calls = 0, &observed](change_request<int>& r) mutable {
            observed = ++calls;
            CHECK_FALSE(r.deferred());
            if (r.value() == 1)
                r.defer();
        });
    target = 1;
    REQUIRE(target == 0);
    target = 2;
    REQUIRE(target == 2);
    REQUIRE(observed == 2);
}

TEST_CASE("interceptor reset keeps its executing callback alive")
{
    property<int> target{0};
    interceptor_handle<int> channel;
    auto token = std::make_shared<int>(42);
    std::weak_ptr<int> weak = token;
    channel = target.intercept([token, &channel, &weak](change_request<int>&) {
        channel.reset();
        CHECK_FALSE(weak.expired());
        CHECK(*token == 42);
    });
    token.reset();
    target = 1;
    REQUIRE(target == 0);
    REQUIRE(weak.expired());
    target = 2;
    REQUIRE(target == 2);
}

TEST_CASE("interceptor replacement during a nested request is safe")
{
    property<int> target{0};
    interceptor_handle<int> channel;
    int calls = 0;
    channel = target.intercept([&](change_request<int>& r) {
        ++calls;
        if (r.value() == 1)
            target = 2;
        else
        {
            channel.reset();
            channel = target.intercept([](change_request<int>&) {});
            target = 3;
        }
    });
    target = 1;
    REQUIRE(target == 3);
    REQUIRE(calls == 2);
    REQUIRE(channel.valid());
}

TEST_CASE("interceptor revocation unwinds safely on exception")
{
    property<int> target{0};
    interceptor_handle<int> channel;
    auto token = std::make_shared<int>(42);
    std::weak_ptr<int> weak = token;
    channel = target.intercept([token, &channel](change_request<int>&) {
        channel.reset();
        throw std::runtime_error{"callback"};
    });
    token.reset();
    REQUIRE_THROWS_AS(target.assign(1), std::runtime_error);
    REQUIRE(weak.expired());
    target = 2;
    REQUIRE(target == 2);
}

TEST_CASE("interceptor rejects an empty callback")
{
    property<int> target{0};
    REQUIRE_THROWS_AS(target.intercept({}), std::invalid_argument);
}

TEST_CASE("publication guards survive nested writes and destruction")
{
    auto target = std::make_unique<property<int>>(0);
    int changed = 0;
    target->value_changed() += [&] { ++changed; };
    target->value_changing() += [&] {
        if (**target == 1)
            *target = 2;
        else
            target.reset();
    };
    *target = 1;
    REQUIRE_FALSE(target);
    REQUIRE(changed == 0);
}

TEST_CASE("publication guards unwind when a callback throws")
{
    property<int> target{0};
    auto throwing = target.value_changing().connect(
        [] { throw std::runtime_error{"callback"}; });
    REQUIRE_THROWS_AS(target.assign(1), std::runtime_error);
    throwing.disconnect();
    int changed = 0;
    target.value_changed() += [&] { ++changed; };
    target = 2;
    REQUIRE(target == 2);
    REQUIRE(changed == 1);
}

TEST_CASE("moving during publication cancels the old notification")
{
    property<int> source{0};
    std::optional<property<int>> destination;
    int changed = 0;
    source.value_changed() += [&] { ++changed; };
    auto moving = source.value_changing().connect(
        [&] { destination.emplace(std::move(source)); });
    source = 1;
    REQUIRE(destination->get() == 1);
    REQUIRE(changed == 0);
    moving.disconnect();
    *destination = 2;
    REQUIRE(changed == 1);
}

TEST_CASE("value_changing exposes the new value for binding propagation")
{
    property<int> source{1};
    property<int> target = BIND(source, source * 2);
    int observed = 0;
    source.value_changing() += [&] { observed = *source; };
    source.value_changed() += [&] { REQUIRE(target == *source * 2); };
    source = 3;
    REQUIRE(observed == 3);
    REQUIRE(target == 6);
}

TEST_CASE("interception defers publication and survives assignments")
{
    property<int> source{1}, target = BIND_EQ(source);
    int requested = 0, changed = 0;
    auto channel = target.intercept([&](change_request<int>& request) {
        requested = request.value();
        request.defer();
    });
    target.value_changed() += [&] { ++changed; };
    source = 10;
    REQUIRE(requested == 10);
    REQUIRE(target == 1);
    REQUIRE(changed == 0);
    channel.publish(5);
    REQUIRE(target == 5);
    REQUIRE(changed == 1);
    source = 20;
    REQUIRE(requested == 20);
    target = 5;
    REQUIRE(requested == 5);
    source = 30;
    REQUIRE(requested == 5);
    channel.reset();
    REQUIRE_FALSE(channel.publish(99));
    target = 9;
    REQUIRE(target == 9);
}

TEST_CASE(
    "immediate intercepted binding preserves two phase dependency updates")
{
    property<int> source{1}, target = BIND_EQ(source);
    auto channel = target.intercept([](change_request<int>&) {});
    property<int> twice = BIND((target, t), t * 2);
    int changed = 0;
    target.value_changed() += [&] {
        REQUIRE(twice == *target * 2);
        ++changed;
    };
    source = 10;
    REQUIRE(target == 10);
    REQUIRE(changed == 1);
    {
        transaction update{source};
        source = 20;
        source = 30;
        update.commit();
    }
    REQUIRE(twice == 60);
    REQUIRE(changed == 2);
}

TEST_CASE("interception follows moves and invalidates old destination")
{
    property<int> a{1}, b{2};
    int invalidated = 0;
    auto first = a.intercept([](change_request<int>& r) { r.defer(); });
    auto second = b.intercept([](change_request<int>& r) { r.defer(); },
                              [&] { ++invalidated; });
    b = std::move(a);
    REQUIRE(invalidated == 1);
    REQUIRE_FALSE(second.valid());
    REQUIRE(first.publish(7));
    REQUIRE(b == 7);
    property<int> c{b};
    c = 8;
    REQUIRE(c == 8);
    REQUIRE(b == 7);
}

TEST_CASE("interception and signal support destruction inside publication")
{
    auto target = std::make_unique<property<int>>(0);
    auto channel = target->intercept([](change_request<int>& r) { r.defer(); });
    bool late_slot = false;
    target->value_changing() += [&] { target.reset(); };
    target->value_changing() += [&] { late_slot = true; };
    channel.publish(1);
    REQUIRE_FALSE(channel.valid());
    REQUIRE_FALSE(late_slot);
    REQUIRE_FALSE(channel.publish(2));
}

TEST_CASE(
    "interception can be revoked or its property destroyed inside request")
{
    auto target = std::make_unique<property<int>>(0);
    auto channel = target->intercept(
        [&](change_request<int>& request) { target.reset(); });
    *target = 10;
    REQUIRE_FALSE(channel.valid());
}

TEST_CASE("interception rejects duplicates and is not copied")
{
    property<int> target{0};
    auto channel = target.intercept([](change_request<int>& r) { r.defer(); });
    REQUIRE_THROWS_AS(target.intercept([](change_request<int>&) {}),
                      std::logic_error);
    property<int> copy{target};
    copy = 100;
    REQUIRE(copy == 100);
    target = 100;
    REQUIRE(target == 0);
}

TEST_CASE("a nested interception request supersedes its outer request")
{
    property<int> target{0};
    auto channel = target.intercept([&](change_request<int>& request) {
        if (request.value() == 1)
            target = 2;
    });
    target = 1;
    REQUIRE(target == 2);
}
