#include <circle/reactive/ptr.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("ptr")
{
    circle::ptr<int> pint = circle::make_ptr<int>();
    *pint = 5;
    REQUIRE(*pint == 5);

    SECTION("move constructor")
    {
        auto p2 = std::move(pint);
        REQUIRE(*p2 == 5);
    }

    SECTION("move constructor")
    {
        auto destroy1_called = false;
        auto destroy2_called = false;
        pint.before_destroyed().connect([&] { destroy1_called = true; });
        {
            auto p2 = std::move(pint);
            REQUIRE(destroy1_called == false);
            p2.before_destroyed().connect([&] { destroy2_called = true; });
        }
        REQUIRE(destroy1_called == true);
        REQUIRE(destroy2_called == true);
    }
}

TEST_CASE("tracking_ptr")
{
    circle::ptr<int> pint = circle::make_ptr<int>();
    auto weak = circle::tracking_ptr{pint};
    *pint = 5;
    REQUIRE(*weak == 5);

    SECTION("move constructor")
    {
        auto p2 = std::move(weak);
        REQUIRE(*p2 == 5);
    }

    SECTION("move & copy constructor")
    {
        auto destroy1_called = false;
        auto destroy2_called = false;
        auto destroy3_called = false;
        weak.before_destroyed().connect([&] { destroy1_called = true; });
        auto p_moved = std::move(weak);
        p_moved.before_destroyed().connect([&] { destroy2_called = true; });
        auto p_copy = p_moved;
        p_copy.before_destroyed().connect([&] { destroy3_called = true; });
        {
            auto will_be_deleted = std::move(pint);
            REQUIRE(destroy1_called == false);
            REQUIRE(destroy2_called == false);
            REQUIRE(destroy3_called == false);
        }
        REQUIRE(destroy1_called == true);
        REQUIRE(destroy2_called == true);
        REQUIRE(destroy3_called == true);
    }
}
