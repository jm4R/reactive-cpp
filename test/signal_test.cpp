#include <circle/bind/signal.hpp>

#include <catch2/catch.hpp>

using namespace circle;

int global_int;

TEST_CASE("signal")
{
    SECTION("empty")
    {
        signal<int> s;
        s.emit(5);
    }

    SECTION("lambda")
    {
        int res{};

        signal<int> s;
        s.connect([&](int v){ res = v; });
        REQUIRE(res == 0);
        s.emit(5);
        REQUIRE(res == 5);
    }

    SECTION("function pointer")
    {
        global_int = 0;

        signal<int> s;
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

        signal<int> s;
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

        signal<int> s;
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

        signal<int> s;
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

        signal<int> s;
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

        signal<int> s;
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

        signal<int, char> s;
        s.connect([&](int v){ res = v; });
        REQUIRE(res == 0);
        s.emit(5, 'a');
        REQUIRE(res == 5);
    }

    SECTION("provide first parameters by value")
    {
        int res{};

        signal<int> s;
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
        signal<int> s;

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

        signal<int> s;
        s.connect(static_cast<void (lstr::*)(int)>(&lstr::foo), &obj);
        REQUIRE(global_int == 0);
        s.emit(5);
        REQUIRE(global_int == 2);
    }

    SECTION("move constructor")
    {
        int res{};

        signal<int> s1;
        s1.connect([&](int v){ res = v; });
        auto s2 = std::move(s1);
        s2.emit(5);
        REQUIRE(res == 5);
    }

    SECTION("move assignment operator")
    {
        int res{};

        signal<int> s1;
        s1.connect([&](int v){ res = v; });
        signal<int> s2;
        s2 = std::move(s1);
        s2.emit(5);
        REQUIRE(res == 5);
    }
}

TEST_CASE("connection")
{
    SECTION("simple")
    {
        int res{};

        signal<int> s;
        connection c = s.connect([&](int v){ res = v; });
        s.emit(5);
        c.disconnect();
        s.emit(10);
        REQUIRE(res == 5);
    }

    SECTION("copy")
    {
        int res{};

        signal<int> s;
        connection c = s.connect([&](int v){ res = v; });
        s.emit(5);
        auto c2 = c;
        c2.disconnect();
        s.emit(10);
        REQUIRE(res == 5);
    }

    SECTION("move")
    {
        int res{};

        signal<int> s;
        connection c = s.connect([&](int v){ res = v; });
        s.emit(5);
        auto c2 = std::move(c);
        c2.disconnect();
        s.emit(10);
        REQUIRE(res == 5);
    }

    SECTION("belongs_to")
    {
        signal<int> s1;
        signal<float> s2;
        connection c1 = s1.connect([](){});
        connection c2 = s2.connect([](){});
        REQUIRE(c1.belongs_to(s1));
        REQUIRE(c2.belongs_to(s2));
        REQUIRE_FALSE(c1.belongs_to(s2));
        REQUIRE_FALSE(c2.belongs_to(s1));
    }

    SECTION("disconnect invoked during iteration")
    {
        int res1{};
        int res2{};
        int res3{};

        signal<> s;
        connection c1 = s.connect([&] { res1++; });
        connection c2 = s.connect([&] { res2++; });
        connection c3 = s.connect([&] {
            res3++;
            c2.disconnect();
        });

        s();
        REQUIRE(res1 == 1);
        REQUIRE(res2 == 1);
        REQUIRE(res3 == 1);

        s();
        REQUIRE(res1 == 2);
        REQUIRE(res2 == 1);
        REQUIRE(res3 == 2);
    }

    SECTION("disconnect not-invoked during iteration")
    {
        int res1{};
        int res2{};
        int res3{};

        connection* c2ptr{};
        signal<> s;
        connection c1 = s.connect([&] {
            res1++;
            c2ptr->disconnect();
        });
        connection c2 = s.connect([&] { res2++; });
        connection c3 = s.connect([&] { res3++; });
        c2ptr = &c2;

        s();
        REQUIRE(res1 == 1);
        REQUIRE(res2 == 0);
        REQUIRE(res3 == 1);

        s();
        REQUIRE(res1 == 2);
        REQUIRE(res2 == 0);
        REQUIRE(res3 == 2);
    }

    SECTION("self-disconnect during iteration")
    {
        int res1{};
        int res2{};
        int res3{};

        connection* c2ptr{};
        signal<> s;
        connection c1 = s.connect([&] { res1++; });
        connection c2 = s.connect([&] {
            c2ptr->disconnect();
            res2++;
        });
        connection c3 = s.connect([&] { res3++; });
        c2ptr = &c2;

        s();
        REQUIRE(res1 == 1);
        REQUIRE(res2 == 1);
        REQUIRE(res3 == 1);

        s();
        REQUIRE(res1 == 2);
        REQUIRE(res2 == 1);
        REQUIRE(res3 == 2);
    }
}

TEST_CASE("scoped_connection")
{
    SECTION("simple")
    {
        int res{};

        signal<int> s;
        {
            scoped_connection c = s.connect([&](int v){ res = v; });
            s.emit(5);
        }
        s.emit(10);
        REQUIRE(res == 5);
    }

    SECTION("release")
    {
        int res{};

        signal<int> s;
        scoped_connection c = s.connect([&](int v){ res = v; });
        s.emit(5);
        c.release();
        s.emit(10);
        REQUIRE(res == 10);
    }

    SECTION("disconnect manually")
    {
        int res{};

        signal<int> s;
        scoped_connection c = s.connect([&](int v){ res = v; });
        s.emit(5);
        c.disconnect();
        s.emit(10);
        REQUIRE(res == 5);
    }

    SECTION("operator=(connection) disconnects old connection")
    {
        int res{};

        signal<int> s;
        scoped_connection c = s.connect([&](int v) { res += v; });
        c = s.connect([&](int v) { res += v * 100; });
        s.emit(5);
        REQUIRE(res == 500);
    }

    SECTION("operator=(scoped_connection&&) disconnects old connection")
    {
        int res{};

        signal<int> s;
        scoped_connection c = s.connect([&](int v) { res += v; });
        c = scoped_connection{};
        s.emit(5);
        REQUIRE(res == 0);
    }
}