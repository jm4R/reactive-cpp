#include <circle/reactive/transaction.hpp>

#include <circle/reactive/bind.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("transaction")
{
    using namespace circle;
    auto p1 = property{1};
    auto p2 = property{2};
    auto p_const = property{3};
    property sum = BIND(p1, p2, p_const, p1+p2+p_const);

    auto callback_counter = 0;

    auto callback = [&]() {
        REQUIRE(p1 == 10);
        REQUIRE(p2 == 20);
        REQUIRE(p_const == 3);
        ++callback_counter;
    };

    p1.value_changed() += callback;
    p2.value_changed() += callback;
    p_const.value_changed() += callback;

    auto t = transaction{p1, p2, p_const};
    p1 = 10;
    p2 = 20;
    p_const = 3;
    REQUIRE(callback_counter == 0);
    REQUIRE(sum == 6);
    t.commit();
    REQUIRE(callback_counter == 2);
    REQUIRE(sum == 33);
}
