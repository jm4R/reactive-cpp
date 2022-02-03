#include <circle/bind/properties_observer.hpp>

#include <catch2/catch.hpp>

using namespace circle;

TEST_CASE("properties_observer")
{
    static_assert(std::is_constructible_v<properties_observer<property<int>>,
                                          property<int>&>);
    static_assert(std::is_constructible_v<
                  properties_observer<property<int>, property<double>>,
                  property<int>&, property<double>&>);
    static_assert(
        !std::is_constructible_v<properties_observer<double>, double&>);
    static_assert(!std::is_copy_constructible_v<properties_observer<int>>);
    static_assert(!std::is_copy_assignable_v<properties_observer<int>>);
    static_assert(!std::is_move_assignable_v<properties_observer<int>>);
    static_assert(!std::is_move_constructible_v<properties_observer<int>>);
    static_assert(!std::is_move_assignable_v<properties_observer<int>>);

    property<int> a = 1;
    property<long> b = 1;
    properties_observer obs{a, b};
    int c = a * b;
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

}

TEST_CASE("property_proxy")
{
    property<int> p1 = 5;
    property_proxy proxy = p1;
    REQUIRE(proxy == 5);
    p1 = 10;
    REQUIRE(proxy == 10);
    auto p2 = std::move(p1);
    p2 = 15;
    REQUIRE(proxy == 15);
    property<int> p3 = 5;
    REQUIRE(proxy == 15);
    p3 = std::move(p2);
    REQUIRE(proxy == 15);
    p3 = 20;
    REQUIRE(proxy == 20);
}
