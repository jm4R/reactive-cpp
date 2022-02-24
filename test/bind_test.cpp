#include <circle/reactive/bind.hpp>

#include <catch2/catch.hpp>

#include <cmath>
#include <string>

using namespace circle;

static int global_int;

TEST_CASE("binding")
{
    static_assert(std::is_constructible_v<binding<float, int>, property<int>&,
                                          float (*)(const int&)>);
    static_assert(std::is_nothrow_destructible_v<binding<float, int>>);

    property<int> a;
    property<int> b;
    property<int> c;

    SECTION("simple")
    {
        c = make_binding(+[](const int& a, const int& b) { return a * b; }, a, b);
        a = 12;
        b = 100;
        REQUIRE(c == 1200);
    }

    SECTION("macro")
    {
        c = BIND(a, b, std::max(a, b)*2);
        a = 8;
        b = 4;
        REQUIRE(c == 16);

        auto a1 = std::move(a);
        auto c1 = std::move(c);
        auto b1 = std::move(b);


        REQUIRE(c1 == 16);
        b1 = 10;
        REQUIRE(c1 == 20);
        a1 = 100;
        REQUIRE(c1 == 200);
        a = 1000;
        b = 1000;
        REQUIRE(c1 == 200);
    }

    SECTION("bind to deleted property")
    {
        property<int> a = 5;
        property<int> c;
        {
            property<int> b = 10;
            c = BIND(a, b, a * b);
        }
        REQUIRE(c == 50);
        a = 100;
        REQUIRE(c == 50);
    }

    SECTION("dynamic type")
    {
        property max = BIND(a, b, std::max(a, b));
        property str =
            BIND(a, b, max,
                 "max(" + std::to_string(a) + ", " + std::to_string(b) +
                     ") = " + std::to_string(max));

        a = 60;
        b = 75;

        REQUIRE(*str == std::string{"max(60, 75) = 75"});
    }

    SECTION("lazy evaluation")
    {
        global_int = 0;
        c = BIND(a, b, global_int = a + b);
        a = 18;
        b = 4;

        REQUIRE(global_int == 0);
        REQUIRE(c == 22);
        REQUIRE(global_int == 22);
    }
}
