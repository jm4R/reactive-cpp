#include <circle/reactive/ptr.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("ptr")
{
    circle::ptr<int> pint = circle::make_ptr<int>();
    *pint = 5;
    REQUIRE(*pint == 5);

    circle::ptr<std::optional<int>> poptint = circle::make_ptr<std::optional<int>>();
    *poptint = 5;
    REQUIRE(*poptint == 5);
    REQUIRE(**poptint == 5);

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

    SECTION("move assignment")
    {
        auto destroy1_called = false;
        auto destroy2_called = false;
        pint.before_destroyed().connect([&] { destroy1_called = true; });
        {
            auto p2 = circle::ptr<int>{};
            p2 = std::move(pint);
            REQUIRE(destroy1_called == false);
            p2.before_destroyed().connect([&] { destroy2_called = true; });
        }
        REQUIRE(destroy1_called == true);
        REQUIRE(destroy2_called == true);
    }

    SECTION("move constructor with implicit conversion")
    {
        struct base
        {
            int value{};
        };
        struct derived : public base
        {
        };

        auto derived_ptr = circle::make_ptr<derived>();
        derived_ptr->value = 5;
        auto destroyed = false;
        derived_ptr.before_destroyed() += [&] { destroyed = true; };
        circle::ptr<base> base_ptr = std::move(derived_ptr);
        REQUIRE(!destroyed);
        REQUIRE(base_ptr->value == 5);
        base_ptr.reset();
        REQUIRE(destroyed);
    }

    SECTION("reset")
    {
        pint.reset();
        REQUIRE(!pint);
    }

    SECTION("resetting during resetting is NOOP")
    {
        int call_count = 0;
        pint.before_destroyed().connect([&] {
            ++call_count;
            pint.reset();
        });

        pint.reset();
        REQUIRE(call_count == 1);
        REQUIRE(!pint);
    }

    SECTION("assign nullptr")
    {
        pint = nullptr;
        REQUIRE(!pint);
    }

    SECTION("get")
    {
        std::ignore = pint.get();
    }

    SECTION("operators")
    {
        REQUIRE(poptint->emplace(5) == 5);
        REQUIRE(poptint->value_or(5) == 5);
        *poptint = 5;
        REQUIRE(*poptint == 5);
        REQUIRE(poptint != nullptr);
        int b = 5;
        REQUIRE(pint != &b);
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

    SECTION("tracking to moved")
    {
        struct base
        {
            virtual int get() const { return 1; }
        };
        struct derived : public base
        {
            int get() const override { return 2; }
        };

        auto derived_ptr = circle::make_ptr<derived>();
        auto tracking = circle::tracking_ptr{derived_ptr};
        circle::ptr<base> base_ptr = std::move(derived_ptr);
        REQUIRE(base_ptr->get() == 2);
        auto destroyed = false;
        tracking.before_destroyed() += [&] { destroyed = true; };
        base_ptr.reset();
        REQUIRE(destroyed);
    }
}

namespace {

struct trackable_base : public circle::enable_tracking_from_this<trackable_base>
{
    int x{};
};
struct trackable_derived : public trackable_base
{
    trackable_derived() : tracking_from_constructor{tracking_from_this<trackable_derived>()}
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
            ptr->tracking_from_this();
        circle::tracking_ptr<trackable_derived> tracking_derived =
            ptr->tracking_from_this<trackable_derived>();

        circle::tracking_ptr<trackable_base> tracking_base_copy =
            tracking_derived;

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
