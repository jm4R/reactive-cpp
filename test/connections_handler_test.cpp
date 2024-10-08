#include <circle/reactive/connections_handler.hpp>

#include <circle/reactive/property.hpp>

#include <catch2/catch_all.hpp>

using namespace circle;

TEST_CASE("connections_handler")
{
    static bool called{};

    struct test_connections_handler : connections_handler
    {
        test_connections_handler(signal<int>& sig, const property<int>& prop)
        {
            AUTO_CONNECT(sig, on_signal_emitted);
            AUTO_CONNECT(sig, on_signal_emitted_param, 5);
            AUTO_CONNECT(prop, on_property_changed);
            AUTO_CONNECT(prop, on_property_changed_param, 5);
        }

        void on_signal_emitted()
        {
            signal_emitted_called = true;
            called = true;
        }

        void on_signal_emitted_param(int connect_param, int signal_param)
        {
            signal_emitted_param1 = connect_param;
            signal_emitted_param2 = signal_param;
            called = true;
        }

        void on_property_changed()
        {
            property_changed_called = true;
            called = true;
        }

        void on_property_changed_param(int connect_param, int property_val)
        {
            property_changed_param1 = connect_param;
            property_changed_param2 = property_val;
            called = true;
        }

        bool signal_emitted_called{};
        int signal_emitted_param1{};
        int signal_emitted_param2{};
        bool property_changed_called{};
        int property_changed_param1{};
        int property_changed_param2{};
    };

    signal<int> s;
    property<int> p{200};

    {
        test_connections_handler handler{s, p};
        REQUIRE(handler.signal_emitted_called == false);
        REQUIRE(handler.signal_emitted_param1 == 0);
        REQUIRE(handler.signal_emitted_param2 == 0);
        REQUIRE(handler.property_changed_called == true);
        REQUIRE(handler.property_changed_param1 == 5);
        REQUIRE(handler.property_changed_param2 == 200);

        s(10);
        REQUIRE(handler.signal_emitted_called == true);
        REQUIRE(handler.signal_emitted_param1 == 5);
        REQUIRE(handler.signal_emitted_param2 == 10);

        p = 300;
        REQUIRE(handler.property_changed_param1 == 5);
        REQUIRE(handler.property_changed_param2 == 300);
    }

    called = false;
    s(50);
    p = 400;
    REQUIRE(called == false);
}
