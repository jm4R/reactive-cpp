#include <circle/reactive/bind.hpp>

#include <catch2/catch_all.hpp>

#include <cmath>
#include <string>

using namespace circle;

static int global_int;

struct tracked_test
{
    int val{};
};


struct test_struct
{
    property<int> test_member1;
    property<int> test_member2;
    inline static property<int> test_static_member;
};

TEST_CASE("binding", "[binding]")
{
    static_assert(
        std::is_constructible_v<binding<float, property<int>>, property<int>&,
                                float (*)(const int&)>);
    static_assert(
        std::is_nothrow_destructible_v<binding<float, property<int>>>);

    property<int> a;
    property<int> b;
    property<int> c;

    SECTION("simple")
    {
        c = make_binding(
            +[](const int& a, const int& b) { return a * b; }, a, b);
        a = 12;
        b = 100;
        REQUIRE(c == 1200);
        a = 13;
        REQUIRE(c == 1300);
    }

    SECTION("BIND macro")
    {
        c = BIND(a, b, std::max(a, b) * 2);
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

    SECTION("immediate (not lazy) evaluation")
    {
        global_int = 0;
        a = 15;
        b = 2;
        c = BIND(a, b, a / b);

        bool called = false;
        c.value_changed() += [&]{ called = true; };

        a = 14;
        REQUIRE(!called);
        a = 15;
        REQUIRE(!called);
        a = 13;
        REQUIRE(called);
    }

    SECTION("don't call value_changed when re-calculated value didn't change")
    {
        global_int = 0;
        c = BIND(a, b, global_int = a + b);
        a = 18;
        b = 4;

        REQUIRE(global_int == 22);
        REQUIRE(c == 22);
        REQUIRE(global_int == 22);
    }

    SECTION("use property_ref")
    {
        auto a_ref = property_ref{a};
        c = make_binding(
            +[](const int& a, const int& b) { return a * b; }, a_ref, b);
        a = 12;
        b = 100;
        REQUIRE(c == 1200);
        a = 13;
        REQUIRE(c == 1300);
    }

    SECTION("use ptr")
    {
        {
            auto ttt = circle::make_ptr<tracked_test>();
            c = make_binding(
                +[](const tracked_test& ttt, const int& a, const int& b) {
                    return std::max(ttt.val, a);
                },
                ttt, a, b);
            c = BIND(ttt, a, b, std::max(ttt.val, a));

            ttt->val = 150;
            REQUIRE(c == 0); // ttt doesn't notify value change
            a = 120;
            REQUIRE(c == 150);
        }

        a = 200;
        b = 200;
        REQUIRE(c == 150);
    }

    SECTION("use tracking_ptr")
    {
        {
            auto ttt = circle::make_ptr<tracked_test>();
            auto ppp = circle::tracking_ptr{ttt};
            c = make_binding(
                +[](const tracked_test& ttt, const int& a, const int& b) {
                    return std::max(a, b);
                },
                ppp, a, b);
            c = BIND(ppp, a, b, std::max(a, b));

            b = 110;
            REQUIRE(c == 110);
        }

        a = 200;
        b = 200;
        REQUIRE(c == 110);
    }

    SECTION("custom name parameters")
    {
        test_struct obj;
        test_struct* ptr = &obj;
        property<int> sum =
            BIND((obj.test_member1, arg1), (ptr->test_member2, arg2),
                 (test_struct::test_static_member, arg3), arg1 + arg2 + arg3);

        REQUIRE(sum == 0);
        obj.test_member1 = 1;
        REQUIRE(sum == 1);
        ptr->test_member2 = 10;
        REQUIRE(sum == 11);
        test_struct::test_static_member = 100;
        REQUIRE(sum == 111);
    }

    SECTION("BIND_EQ macro")
    {
        test_struct obj;
        c = BIND_EQ(obj.test_member1);
        obj.test_member1 = 8;
        REQUIRE(c == 8);
    }

    SECTION("bind to const property")
    {
        const property a = 5;
        property b = BIND(a, a * 2);
        REQUIRE(b == 10);
    }

    SECTION("all binding tree is up to date during value_changed")
    {
        // This test also shows why single `value_changed` signal is not enough
        // and `value_changing` had to be introduced:
        //
        // * value_changing: during its propagation all dependent properties
        // values are updated
        //
        // * value_changed: second signal emmited when all dependent properties
        // values are already up to date

        auto x = property{1};
        property x10 = BIND(x, x * 10);
        property x100 = BIND(x, x * 100);
        property xx1000 = BIND(x10, x100, x10 * x100);

        int called = 0;
        auto check = [&] {
            ++called;
            REQUIRE(x10 == x * 10);
            REQUIRE(x100 == x * 100);
            REQUIRE(xx1000 == x * x * 1000);
        };

        x10.value_changed() += check;
        x100.value_changed() += check;
        xx1000.value_changed() += check;

        x = 2;
        x = 3;
        REQUIRE(called == 6);
    }
}
