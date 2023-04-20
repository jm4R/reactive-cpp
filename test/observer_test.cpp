#include <circle/reactive/observer.hpp>
#include <circle/reactive/property.hpp>

#include <catch2/catch.hpp>

using namespace circle;

TEST_CASE("observer with tracking_ptr")
{
    static_assert(std::is_constructible_v<observer<1>,
                                          property_ptr<int>&>);
    static_assert(
        std::is_constructible_v<observer<2>,
                                property_ptr<int>&, property_ptr<double>&>);
    static_assert(!std::is_copy_constructible_v<observer<2>>);
    static_assert(!std::is_copy_assignable_v<observer<2>>);
    static_assert(!std::is_move_assignable_v<observer<2>>);
    static_assert(!std::is_move_constructible_v<observer<2>>);
    static_assert(!std::is_move_assignable_v<observer<2>>);

    property<int> a = 1;
    property<long> b = 1;
    observer obs{property_ptr{&a}, property_ptr{&b}};
    long c = a * b;
    obs.set_callback([&] { c = a * b; });
    REQUIRE(c == 1);
    a = 2;
    REQUIRE(c == 2);
    b = 5;
    REQUIRE(c == 10);

    bool called = false;
    obs.set_callback([&] { called = true; });
    auto b2 = std::move(b);
    REQUIRE_FALSE(called);
    b = 10;
    REQUIRE_FALSE(called);
    b2 = 10;
    REQUIRE(called);

    bool destroyed{};
    obs.set_destroyed_callback([&]{ destroyed = true; });
    REQUIRE_FALSE(destroyed);

    {
        auto b3 = std::move(b2);
    }

    REQUIRE(destroyed);
}

TEST_CASE("observer with raw pointer")
{
    property<int> a = 1;
    property<long> b = 1;
    observer obs{&a, &b};
    long c = a * b;
    obs.set_callback(
        [&c, a = property_ptr{&a}, b = property_ptr{&b}] { c = *a * *b; });
    REQUIRE(c == 1);
    a = 2;
    REQUIRE(c == 2);
    b = 5;
    REQUIRE(c == 10);

    bool called = false;
    obs.set_callback([&] { called = true; });
    auto b2 = std::move(b);
    REQUIRE_FALSE(called);
    b = 10;
    REQUIRE_FALSE(called);
    b2 = 10;
    REQUIRE(called);

    bool destroyed{};
    obs.set_destroyed_callback([&]{ destroyed = true; });
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
    observer obs{&a, &b};
    long c = a * b;
    obs.set_callback(
        [&c, a = property_ptr{&a}, b = property_ptr{&b}] { c = *a * *b; });

    bool called = false;
    obs.set_callback([&] { called = true; });
    auto b2 = std::move(b);
    REQUIRE_FALSE(called);
    b = 10;
    REQUIRE_FALSE(called);
    b2 = 10;
    REQUIRE(called);
}
