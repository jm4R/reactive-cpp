#include <circle/reactive/observer.hpp>
#include <circle/reactive/property.hpp>

#include <catch2/catch_all.hpp>

using namespace circle;

TEST_CASE("observer with property")
{
    static_assert(std::is_constructible_v<observer<1>, property_ref<int>&>);
    static_assert(std::is_constructible_v<observer<2>, property_ref<int>&,
                                          property_ref<double>&>);
    static_assert(!std::is_copy_constructible_v<observer<2>>);
    static_assert(!std::is_copy_assignable_v<observer<2>>);
    static_assert(!std::is_move_assignable_v<observer<2>>);
    static_assert(!std::is_move_constructible_v<observer<2>>);
    static_assert(!std::is_move_assignable_v<observer<2>>);

    property<int> a = 1;
    property<long> b = 1;
    observer obs{a, b};
    long c = a * b;
    obs.set_changed_callback([&] { c = a * b; });
    REQUIRE(c == 1);
    a = 2;
    REQUIRE(c == 2);
    b = 5;
    REQUIRE(c == 10);

    bool called = false;
    obs.set_changed_callback([&] { called = true; });
    auto b2 = std::move(b);
    REQUIRE_FALSE(called);
    b = 10;
    REQUIRE_FALSE(called);
    b2 = 10;
    REQUIRE(called);

    bool destroyed{};
    obs.set_destroyed_callback([&] { destroyed = true; });
    REQUIRE_FALSE(destroyed);

    {
        auto b3 = std::move(b2);
    }

    REQUIRE(destroyed);
}

TEST_CASE("observer with property_ref")
{
    property<int> a = 1;
    property<long> b = 1;
    observer obs{a, b};
    long c = a * b;
    obs.set_changed_callback(
        [&c, a = property_ref{a}, b = property_ref{b}] { c = *a * *b; });
    REQUIRE(c == 1);
    a = 2;
    REQUIRE(c == 2);
    b = 5;
    REQUIRE(c == 10);

    bool called = false;
    obs.set_changed_callback([&] { called = true; });
    auto b2 = std::move(b);
    REQUIRE_FALSE(called);
    b = 10;
    REQUIRE_FALSE(called);
    b2 = 10;
    REQUIRE(called);

    bool destroyed{};
    obs.set_destroyed_callback([&] { destroyed = true; });
    REQUIRE_FALSE(destroyed);

    {
        auto b3 = std::move(b2);
    }

    REQUIRE(destroyed);
}

TEST_CASE("almost binding")
{
    property<int> a = 1;
    property<long> b = 1;
    observer obs{a, b};
    long c = a * b;
    obs.set_changed_callback(
        [&c, a = property_ref{a}, b = property_ref{b}] { c = *a * *b; });

    bool called = false;
    obs.set_changed_callback([&] { called = true; });
    auto b2 = std::move(b);
    REQUIRE_FALSE(called);
    b = 10;
    REQUIRE_FALSE(called);
    b2 = 10;
    REQUIRE(called);
}
