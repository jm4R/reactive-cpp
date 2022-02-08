#include <circle/reactive/bind.hpp>

#include <catch2/catch.hpp>

#include <cmath>
#include <string>

using namespace circle;

TEST_CASE("binding")
{
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
}
