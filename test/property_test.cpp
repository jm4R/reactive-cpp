#include <circle/bind/property.hpp>

#include <catch2/catch.hpp>

using namespace circle;

TEST_CASE("property")
{
    SECTION("default-constructed")
    {
        circle::property<int> p;
        REQUIRE(p == 0);
    }

    SECTION("value-constructed")
    {
        circle::property<int> p{5};
        REQUIRE(p == 5);
    }

    SECTION("operator=")
    {
        circle::property<int> p{};
        p = 5;
        REQUIRE(p == 5);
    }

    SECTION("value_changed")
    {
        circle::property<int> p{};
        int new_value = 0;
        p.value_changed().connect([&](int val) { new_value = val; });

        SECTION("changed by operator=")
        {
            p = 5;
            REQUIRE(new_value == 5);
        }

        SECTION("changed after moved")
        {
            circle::property<int> p2 = std::move(p);
            p2 = 5;
            REQUIRE(new_value == 5);
        }
    }

    SECTION("moved")
    {
        circle::property<int> p{};
        circle::property<int>* addr = &p;
        p.moved().connect([&](property<int>& pl) { addr = &pl; });

        SECTION("moved by move constructor")
        {
            circle::property<int> p2 = std::move(p);
            REQUIRE(addr == &p2);
        }

        SECTION("moved by move assignment operator")
        {
            circle::property<int> p2;
            p2 = std::move(p);
            REQUIRE(addr == &p2);
        }
    }
}

struct test_provider final : circle::value_provider<int>
{
    circle::signal<> updated_;
    int value_{};
    signal<>& updated() override { return updated_; }
    int get() override { return value_; }

    test_provider(int initial_value) : value_{initial_value} {};

    inline static test_provider* instance{};

    static circle::value_provider_ptr<int> make(int initial_value)
    {
        auto ptr = std::make_unique<test_provider>(initial_value);
        instance = ptr.get();
        return ptr;
    }
};

TEST_CASE("property with value_provider")
{
    circle::property<int> p;
    int new_value = 0;
    bool change_called = false;
    p.value_changed().connect([&](int val) {
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
        provider->updated_.emit();
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
        provider->updated_.emit();
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
        provider->updated_.emit();
        REQUIRE(change_called == true);
        REQUIRE(new_value == 5);
        REQUIRE(p == 5);
    }
}

TEST_CASE("property_ref")
{
    property<int> p1 = 5;
    property_ref ref = p1;
    REQUIRE(ref == 5);

    SECTION("simple")
    {
        p1 = 10;
        REQUIRE(ref == 10);
        auto p2 = std::move(p1);
        p2 = 15;
        REQUIRE(ref == 15);
        property<int> p3 = 5;
        REQUIRE(ref == 15);
        p3 = std::move(p2);
        REQUIRE(ref == 15);
        p3 = 20;
        REQUIRE(ref == 20);
    }

    SECTION("copy")
    {
        property_ref ref2 = ref;
        p1=10;
        REQUIRE(ref == 10);
        REQUIRE(ref2 == 10);
    }

    SECTION("move")
    {
        property<int> p2 = 100;
        property_ref ref2 = p2;
        ref2 = std::move(ref);
        p1=10;
        p2 = 200;
        REQUIRE(ref2 == 10);
    }
}