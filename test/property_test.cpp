#include <circle/reactive/property.hpp>

#include <catch2/catch_all.hpp>

#include <string>

using namespace circle;

struct noncomparable
{
    int a;
    int b;
};

TEST_CASE("property")
{
    static_assert(
        std::is_nothrow_default_constructible_v<property<std::string>>);
    static_assert(std::is_nothrow_move_constructible_v<property<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<property<std::string>>);
    static_assert(std::is_nothrow_destructible_v<property<std::string>>);

    SECTION("default-constructed")
    {
        property<int> p;
        REQUIRE(p == 0);
    }

    SECTION("value-constructed")
    {
        property<int> p{5};
        REQUIRE(p == 5);
    }

    SECTION("operator=")
    {
        property<int> p{};
        p = 5;
        REQUIRE(p == 5);
    }

    SECTION("value_changed")
    {
        property<int> p{};
        int new_value = 0;
        bool changed = false;
        p.value_changed().connect([&](int val) {
            changed = true;
            new_value = val;
        });

        SECTION("changed by operator=")
        {
            p = 5;
            REQUIRE(new_value == 5);
        }

        SECTION("changed after moved")
        {
            property<int> p2 = std::move(p);
            p2 = 5;
            REQUIRE(new_value == 5);
        }

        SECTION("after assign equal value")
        {
            p = 0;
            REQUIRE_FALSE(changed);
            REQUIRE(new_value == 0);
        }
    }

    SECTION("moved")
    {
        property<int> p{};
        property<int>* addr = &p;
        p.moved().connect([&](property<int>& pl) { addr = &pl; });

        SECTION("moved by move constructor")
        {
            property<int> p2 = std::move(p);
            REQUIRE(addr == &p2);
        }

        SECTION("moved by move assignment operator")
        {
            property<int> p2;
            p2 = std::move(p);
            REQUIRE(addr == &p2);
        }
    }

    SECTION("before_destroyed")
    {
        int val{};
        {
            property<int> p{};
            p.before_destroyed().connect([&](property<int>& pl) { val = pl; });
            p = 5;
            REQUIRE(val == 0);
        }

        REQUIRE(val == 5);
    }

    SECTION("non-comparable type")
    {
        property<noncomparable> p = {};
        bool changed = false;
        p.value_changed().connect([&] { changed = true; });

        p = noncomparable{1, 2};
        REQUIRE(changed);
        changed = false;
        p = noncomparable{1, 2};
        REQUIRE(changed);
        REQUIRE(p->a == 1);
        REQUIRE(p->b == 2);
    }

    SECTION("is up-to-date when moved signal called")
    {
        int res{};
        property<int> p1 = 5;
        p1.moved().connect([&](int val) { res = val; });

        SECTION("by constructor")
        {
            property<int> p2 = std::move(p1);
            REQUIRE(res == 5);
        }

        SECTION("by operator=")
        {
            property<int> p2;
            p2 = std::move(p1);
            REQUIRE(res == 5);
        }
    }

    SECTION("is up-to-date when before_destroyed signal called")
    {

        int res{};
        {
            property<int> p1;
            p1.before_destroyed().connect([&](int val) { res = val; });
            p1 = 5;
        }
        REQUIRE(res == 5);
    }

    SECTION("using const property")
    {
        property<int> p{};

        SECTION("changed by operator=")
        {
            const auto& const_p = p;

            int new_value = 0;
            const_p.value_changed().connect([&](int val) {
                new_value = val;
            });
            p = 5;
            REQUIRE(new_value == 5);
        }
    }

    SECTION("construct / assign r-value")
    {
        std::vector<float> val1{10, 5.0f};
        const auto* raw1 = val1.data();
        property p = std::move(val1);
        REQUIRE(p->data() == raw1);

        std::vector<float> val2{20, 6.0f};
        const auto* raw2 = val2.data();
        p = std::move(val2);
        REQUIRE(p->data() == raw2);
    }

    SECTION("use underlying assignment")
    {
        std::vector<float> val{10, 5.0f};
        property p = val;
        const auto* raw = p->data();
        val[0] = 1.0f;
        p = val;
        REQUIRE(p->data() == raw);
    }
}

struct test_provider final : value_provider<int>
{
    std::function<void()> updated_;
    std::function<void()> before_invalid_;
    int value_{};
    bool is_invalid_{};
    void set_updated_callback(std::function<void()> clb) override
    {
        updated_ = std::move(clb);
    }
    void set_before_invalid_callback(std::function<void()> clb) override
    {
        before_invalid_ = std::move(clb);
    }
    int get() override
    {
        if (is_invalid_)
            throw "You can't do this now!";
        return value_;
    }

    test_provider(int initial_value) : value_{initial_value} {};

    inline static test_provider* instance{};

    static value_provider_ptr<int> make(int initial_value)
    {
        auto ptr = std::make_unique<test_provider>(initial_value);
        instance = ptr.get();
        return ptr;
    }
};

TEST_CASE("property with value_provider")
{
    property<int> p;
    int new_value = 0;
    bool change_called = false;
    auto c = p.value_changed().connect([&](int val) {
        new_value = val;
        change_called = true;
    });

    SECTION("set provider with current value and change")
    {
        p = test_provider::make(0);
        auto* provider = test_provider::instance;
        REQUIRE(change_called == false);
        REQUIRE(p == 0);
        provider->value_ = 5;
        provider->updated_();
        REQUIRE(change_called == true);
        REQUIRE(new_value == 5);
        REQUIRE(p == 5);
    }

    SECTION("set provider with different value")
    {
        p = test_provider::make(5);
        auto* provider = test_provider::instance;
        REQUIRE(change_called == true);
        REQUIRE(new_value == 5);
        REQUIRE(p == 5);
        provider->value_ = 15;
        provider->updated_();
        REQUIRE(new_value == 15);
        REQUIRE(p == 15);
    }

    SECTION("assume provider's value changed even if didn't")
    {
        p = test_provider::make(5);
        auto* provider = test_provider::instance;
        REQUIRE(change_called == true);
        REQUIRE(new_value == 5);
        REQUIRE(p == 5);

        change_called = false;
        provider->updated_();
        REQUIRE(change_called == true);
        REQUIRE(new_value == 5);
        REQUIRE(p == 5);
    }

    SECTION("invalidate leaves value up-to-date")
    {
        c.disconnect();
        {
            p = test_provider::make(5);
            auto* provider = test_provider::instance;
            provider->value_ = 15;
            provider->updated_();
            provider->before_invalid_();
            // freed:
            // provider->value_ = 30;
            // provider->is_invalid_ = true;
        }
        REQUIRE(p == 15);
    }

    SECTION("working with moved property")
    {
        c.disconnect();
        property<int> p2;
        {
            p = test_provider::make(5);
            p2 = std::move(p);
            auto* provider = test_provider::instance;
            provider->value_ = 15;
            provider->updated_();
            provider->before_invalid_();
            // freed:
            // provider->value_ = 30;
            // provider->is_invalid_ = true;
        }
        REQUIRE(p2 == 15);
    }

    SECTION("detach leaves value up-to-date")
    {
        c.disconnect();
        p = test_provider::make(5);
        p.detach();
        REQUIRE(p == 5);
    }
}

TEST_CASE("property_ref")
{
    static_assert(std::is_nothrow_constructible_v<property_ref<std::string>,
                                                  property<std::string>&>);
    static_assert(
        std::is_nothrow_move_constructible_v<property_ref<std::string>>);
    static_assert(std::is_nothrow_move_assignable_v<property_ref<std::string>>);
    static_assert(
        std::is_nothrow_copy_constructible_v<property_ref<std::string>>);
    static_assert(std::is_nothrow_copy_assignable_v<property_ref<std::string>>);
    static_assert(std::is_nothrow_destructible_v<property_ref<std::string>>);

    property<int> p1 = 5;
    property_ref ptr = p1;
    REQUIRE(*ptr == 5);

    SECTION("simple")
    {
        p1 = 10;
        REQUIRE(*ptr == 10);
        auto p2 = std::move(p1);
        p2 = 15;
        REQUIRE(*ptr == 15);
        property<int> p3 = 5;
        REQUIRE(*ptr == 15);
        p3 = std::move(p2);
        REQUIRE(*ptr == 15);
        p3 = 20;
        REQUIRE(*ptr == 20);
    }

    SECTION("copy")
    {
        property_ref ptr2 = ptr;
        p1 = 10;
        REQUIRE(*ptr == 10);
        REQUIRE(*ptr2 == 10);
    }

    SECTION("move")
    {
        property<int> p2 = 100;
        property_ref ptr2 = p2;
        ptr2 = std::move(ptr);
        p1 = 10;
        p2 = 200;
        REQUIRE(*ptr2 == 10);
    }
}
