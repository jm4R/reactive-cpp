#include <circle/reactive/tracking_reference.hpp>

#include <catch2/catch.hpp>

#include <string>

using namespace circle;

struct tracked_test : public enable_tracking_reference<tracked_test>
{
    using base = enable_tracking_reference<tracked_test>;
    tracked_test() = default;
    ~tracked_test() {
        this->call_before_destroyed();
    }

    tracked_test(tracked_test&& other) : base{std::move(other)}
    {
        this->call_moved();
    }

    tracked_test& operator=(tracked_test&& other)
    {
        base::operator=(std::move(other));
        this->call_moved();
        return *this;
    }
};

using test_ref = tracking_reference<tracked_test>;

TEST_CASE("tracking_reference")
{
    tracked_test t1;

    SECTION("move constructor")
    {
        test_ref ref = t1;
        auto t2 = std::move(t1);
        REQUIRE(ref);
        REQUIRE(&*ref == &t2);
    }

    SECTION("operator=")
    {
        test_ref ref1 = t1;
        tracked_test t2;
        test_ref ref2 = t2;

        t2 = std::move(t1);
        REQUIRE(ref1);
        REQUIRE(ref2);
        REQUIRE(&*ref1 == &t2);
        REQUIRE(&*ref2 == &t2);
    }

    SECTION("destructor")
    {
        test_ref ref = t1;
        {
            tracked_test t2;
            ref = t2;
        }
        REQUIRE(ref.is_dangling());
        REQUIRE_FALSE(ref);
    }

    SECTION("moved & destructor")
    {
        test_ref ref = t1;
        {
            auto t2 = std::move(t1);
        }
        REQUIRE(ref.is_dangling());
        REQUIRE_FALSE(ref);
    }
}