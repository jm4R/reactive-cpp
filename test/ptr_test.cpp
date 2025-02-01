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
    auto tracking = circle::tracking_ptr{pint};
    *pint = 5;
    REQUIRE(*tracking == 5);

    SECTION("move constructor")
    {
        auto p2 = std::move(tracking);
        REQUIRE(*p2 == 5);
    }

    SECTION("move & copy constructor")
    {
        auto destroy1_called = false;
        auto destroy2_called = false;
        auto destroy3_called = false;
        tracking.before_destroyed().connect([&] { destroy1_called = true; });
        auto p_moved = std::move(tracking);
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

    SECTION("compare")
    {
        circle::ptr<int> pint2 = circle::make_ptr<int>();

        SECTION("ptr with ptr")
        {
            REQUIRE(pint == pint);
            REQUIRE_FALSE(pint == pint2);
            REQUIRE(pint2 != tracking);
            REQUIRE_FALSE(pint != pint);
        }

        SECTION("ptr with tracking_ptr")
        {
            REQUIRE(tracking == pint);
            REQUIRE(pint == tracking);
            REQUIRE_FALSE(pint2 == tracking);
            REQUIRE_FALSE(pint != tracking);
            REQUIRE(pint2 != tracking);
        }

        SECTION("tracking_ptr with tracking_ptr")
        {
            const auto tracking2 = circle::tracking_ptr{pint2};
            REQUIRE(tracking == tracking);
            REQUIRE_FALSE(tracking2 == tracking);
            REQUIRE_FALSE(tracking != tracking);
            REQUIRE(tracking2 != tracking);
        }
    }
}

namespace {

struct trackable_base : public circle::enable_tracking_from_this<trackable_base>
{
    int x{};
};
struct trackable_derived : public trackable_base
{
    trackable_derived() : tracking_from_constructor{tracking_form_this<trackable_derived>()}
    {
        tracking_from_constructor.before_destroyed().connect(
            [this] { on_before_destroyed(); });
    }

    void on_before_destroyed() { destroyed = true; }

    inline static bool destroyed{};
    circle::tracking_ptr<trackable_derived> tracking_from_constructor;
};

} // namespace

TEST_CASE("tracking from this")
{
    bool local_destroyed = false;
    trackable_derived::destroyed = false;
    {
        auto ptr = circle::make_ptr<trackable_derived>();
        circle::tracking_ptr<trackable_base> tracking_base =
            ptr->tracking_form_this();
        circle::tracking_ptr<trackable_derived> tracking_derived =
            ptr->tracking_form_this<trackable_derived>();

        ptr->tracking_from_constructor.before_destroyed() +=
            [&] { local_destroyed = true; };

        ptr->x = 5;
        REQUIRE(tracking_base->x == 5);

        SECTION("implicitly convertible derived to base")
        {
            circle::tracking_ptr<trackable_base> b = tracking_derived;
            b = tracking_derived;
            REQUIRE(b->x == 5);
        }

        SECTION("cast base to derived")
        {
            circle::tracking_ptr<trackable_derived> d =
                circle::static_pointer_cast<trackable_derived>(tracking_base);
            REQUIRE(d->x == 5);
        }

        REQUIRE_FALSE(trackable_derived::destroyed);
    }
    REQUIRE(trackable_derived::destroyed);
    REQUIRE(local_destroyed);
}
