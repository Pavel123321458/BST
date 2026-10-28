#pragma once
#include <iostream>
#include <stdexcept>
#include <string>

inline void check(bool condition, const char* expression, int line) {
    if (!condition) throw std::runtime_error(std::string("line ") + std::to_string(line) + ": " + expression);
}
#define CHECK(...) check((__VA_ARGS__), #__VA_ARGS__, __LINE__)

template<class Exception, class F> void expect_throw(F action) {
    try { action(); }
    catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception was not thrown");
}

struct TestRunner {
    int passed = 0, failed = 0;
    template<class F> void run(const char* name, F test) {
        try { test(); ++passed; std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failed; std::cout << "FAIL " << name << ": " << e.what() << '\n'; }
    }
    int finish() const {
        std::cout << "Passed: " << passed << "; failed: " << failed << '\n';
        return failed ? 1 : 0;
    }
};
