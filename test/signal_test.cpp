#include <circle/bind/signal.hpp>

#include <catch2/catch.hpp>

using namespace circle;

int global_int;

TEST_CASE("signal")
{
    SECTION("empty")
    {
        circle::signal<int> s;
        s.emit(5);
    }

    SECTION("lambda")
    {
        int res{};

        circle::signal<int> s;
        s.connect([&](int v){ res = v; });
        REQUIRE(res == 0);
        s.emit(5);
        REQUIRE(res == 5);
    }

    SECTION("function pointer")
    {
        global_int = 0;

        circle::signal<int> s;
        s.connect(+[](int v){ global_int = v; });
        REQUIRE(global_int == 0);
        s.emit(5);
        REQUIRE(global_int == 5);
    }

    SECTION("member function")
    {
        struct lstr
        {
            void foo(int v) { res = v; }
            int res{};
        } obj;

        circle::signal<int> s;
        s.connect(&lstr::foo, &obj);
        REQUIRE(obj.res == 0);
        s.emit(5);
        REQUIRE(obj.res == 5);
    }

    SECTION("const member function")
    {
        struct lstr
        {
            void foo(int v) const { res = v; }
            mutable int res{};
        } obj;

        circle::signal<int> s;
        s.connect(&lstr::foo, &obj);
        REQUIRE(obj.res == 0);
        s.emit(5);
        REQUIRE(obj.res == 5);
    }

    SECTION("makes deep copy of functor")
    {
        struct lstr
        {
            lstr() = default;
            lstr(const lstr&) { res = 10; }
            void operator()(int v)
            {
                res = v;
                global_int = v;
            }
            int res{};
        } obj;

        global_int = 0;

        circle::signal<int> s;
        s.connect(obj);
        REQUIRE(obj.res == 0);
        REQUIRE(global_int == 0);
        s.emit(5);
        REQUIRE(obj.res == 0);
        REQUIRE(global_int == 5);
    }

    SECTION("makes deep copy of functor with const operator")
    {
        struct lstr
        {
            lstr() = default;
            lstr(const lstr&) { res = 10; }
            void operator()(int v) const
            {
                res = v;
                global_int = v;
            }
            mutable int res{};
        } obj;

        global_int = 0;

        circle::signal<int> s;
        s.connect(obj);
        REQUIRE(obj.res == 0);
        REQUIRE(global_int == 0);
        s.emit(5);
        REQUIRE(obj.res == 0);
        REQUIRE(global_int == 5);
    }

    SECTION("various value categories parameters")
    {
        int res{};

        circle::signal<int> s;
        s.connect([&](int v){ res = v; });
        REQUIRE(res == 0);
        s.emit(5);
        REQUIRE(res == 5);
        {
            int val = 105;
            s.emit(val);
            REQUIRE(res == 105);
            val = 110;
            s.emit(std::move(val));
            REQUIRE(res == 110);
        }
        {
            const int val = 205;
            s.emit(val);
            REQUIRE(res == 205);
        }
    }

    SECTION("ignore parameter")
    {
        int res{};

        circle::signal<int, char> s;
        s.connect([&](int v){ res = v; });
        REQUIRE(res == 0);
        s.emit(5, 'a');
        REQUIRE(res == 5);
    }

    SECTION("provide first parameters by value")
    {
        int res{};

        circle::signal<int> s;
        int a = 1000;
        s.connect([&](int a, int b, int v) { res = a + b + v; }, a, 100);
        REQUIRE(res == 0);
        s.emit(5);
        REQUIRE(res == 1105);
        a = 0;
        s.emit(6);
        REQUIRE(res == 1106);
    }

    SECTION("take better matched overload")
    {
        struct lstr
        {
            void operator()() { global_int = 1; }
            void operator()(int) { global_int = 2; }
            void operator()(int, int, int) { global_int = 3; }
        } obj;

        global_int = 0;
        circle::signal<int> s;

        SECTION("exact match")
        {
            s.connect(obj);
            REQUIRE(global_int == 0);
            s.emit(5);
            REQUIRE(global_int == 2);
        }

        SECTION("additional arg but ignored")
        {
            s.connect(obj, 1);
            REQUIRE(global_int == 0);
            s.emit(5);
            REQUIRE(global_int == 2);
        }

        SECTION("additional args used")
        {
            s.connect(obj, 1, 2);
            REQUIRE(global_int == 0);
            s.emit(5);
            REQUIRE(global_int == 3);
        }
    }

    SECTION("use specified overload for member function")
    {
        struct lstr
        {
            void foo() { global_int = 1; }
            void foo(int) { global_int = 2; }
            void foo(int, int, int) { global_int = 3; }
        } obj;

        global_int = 0;

        circle::signal<int> s;
        s.connect(static_cast<void (lstr::*)(int)>(&lstr::foo), &obj);
        REQUIRE(global_int == 0);
        s.emit(5);
        REQUIRE(global_int == 2);
    }
}
