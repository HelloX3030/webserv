#pragma once

#include <functional>
#include <iostream>
#include <source_location>
#include <sstream>
#include <string>
#include <vector>

/* test registration.
each TEST macro constructs a static TestRegistrar, whose constructor
pushes into the global registry before main runs. the registry is a
function-local static — avoids the static initialisation order fiasco
across TUs: the vector is guaranteed to exist before any registrar
accesses it. */

struct TestCase
{
    std::string           name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& test_registry()
{
    static std::vector<TestCase> reg;
    return reg;
}

struct TestRegistrar
{
    TestRegistrar(const char* name, std::function<void()> fn)
    {
        test_registry().push_back({name, std::move(fn)});
    }
};

/* the single macro. irreducible: it must define a function and
construct a static registrar with that function's address.
no function call can define a new function. */
#define TEST(name)                                              \
    void test_##name();                                         \
    static TestRegistrar reg_##name(#name, test_##name);        \
    void test_##name()

/* failure type. thrown by assertions, caught by the runner.
abort-on-first-failure within a test: if status is wrong,
checking request fields is undefined — no point continuing. */
struct AssertionFailure : std::exception
{
    std::string msg;
    explicit AssertionFailure(std::string m) : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }
};

/* assertions as template functions.
std::source_location captures caller's file and line at the call site
via the defaulted parameter — no preprocessor needed.
the template parameter requires operator== and operator<< on T. */

template <typename T>
void assert_eq(const T& expected, const T& actual,
               std::source_location loc = std::source_location::current())
{
    if (!(expected == actual))
    {
        std::ostringstream os;
        os << loc.file_name() << ":" << loc.line()
           << "  assert_eq failed"
           << "\n  expected: " << expected
           << "\n  actual:   " << actual;
        throw AssertionFailure(os.str());
    }
}

inline void assert_true(bool cond, const char* expr,
                         std::source_location loc = std::source_location::current())
{
    if (!cond)
    {
        std::ostringstream os;
        os << loc.file_name() << ":" << loc.line()
           << "  assert_true failed: " << expr;
        throw AssertionFailure(os.str());
    }
}

/* assert_true still needs a macro — the expression text (#cond)
can only be captured by the preprocessor. the location is captured
by source_location in the function; the stringification is the
sole reason this macro exists. */
#define ASSERT_TRUE(cond) assert_true((cond), #cond)

/* runner. iterates the registry, catches failures per test.
second catch: prevents a bug in test code (e.g. out-of-range,
null deref) from killing the entire suite. */
inline int run_all_tests()
{
    int passed = 0;
    int failed = 0;

    for (const auto& t : test_registry())
    {
        try
        {
            t.fn();
            ++passed;
        }
        catch (const AssertionFailure& e)
        {
            std::cerr << "FAIL  " << t.name << "\n"
                      << "  " << e.what() << "\n\n";
            ++failed;
        }
        catch (const std::exception& e)
        {
            std::cerr << "FAIL  " << t.name
                      << "  (uncaught: " << e.what() << ")\n\n";
            ++failed;
        }
    }

    std::cerr << "\n" << (passed + failed) << " tests: "
              << passed << " passed, " << failed << " failed\n";
    return (failed > 0) ? 1 : 0;
}
