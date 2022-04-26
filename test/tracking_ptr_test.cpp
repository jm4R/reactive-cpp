#include <circle/reactive/tracking_ptr.hpp>

#include <catch2/catch.hpp>

#include <string>

using namespace circle;

struct tracked_test : public enable_tracking_ptr<tracked_test>
{
    using base = enable_tracking_ptr<tracked_test>;
    tracked_test() = default;
    ~tracked_test() { this->call_before_destroyed(); }

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

using test_ptr = tracking_ptr<tracked_test>;

TEST_CASE("tracking_ptr")
{
    tracked_test t1;

    SECTION("move constructor")
    {
        test_ptr ptr = &t1;
        auto t2 = std::move(t1);
        REQUIRE(ptr);
        REQUIRE(&*ptr == &t2);
    }

    SECTION("operator=")
    {
        test_ptr ptr1 = &t1;
        tracked_test t2;
        test_ptr ptr2 = &t2;

        t2 = std::move(t1);
        REQUIRE(ptr1);
        REQUIRE(ptr2);
        REQUIRE(&*ptr1 == &t2);
        REQUIRE(&*ptr2 == &t2);
    }

    SECTION("destructor")
    {
        test_ptr ptr = &t1;
        {
            tracked_test t2;
            ptr = &t2;
        }
        REQUIRE(ptr.is_dangling());
        REQUIRE_FALSE(ptr);
    }

    SECTION("moved & destructor")
    {
        test_ptr ptr = &t1;
        {
            auto t2 = std::move(t1);
        }
        REQUIRE(ptr.is_dangling());
        REQUIRE_FALSE(ptr);
    }

    SECTION("compare")
    {
        test_ptr ptr;
        test_ptr ptr2;
        REQUIRE(!ptr);
        REQUIRE(ptr != &t1);
        REQUIRE(ptr == nullptr);
        REQUIRE(ptr == ptr2);

        ptr = &t1;
        REQUIRE(ptr);
        REQUIRE(ptr == &t1);
        REQUIRE(ptr != nullptr);
        REQUIRE(ptr != ptr2);

        ptr2 = &t1;
        REQUIRE(ptr == ptr2);
    }
}