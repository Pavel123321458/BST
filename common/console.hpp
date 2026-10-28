#pragma once
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace cpp_labs {
struct EndOfInput {};
inline void setup_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}
inline std::string line(const std::string& prompt) {
    std::cout << prompt;
    std::string result;
    if (!std::getline(std::cin, result)) throw EndOfInput{};
    return result;
}
template<class T> T number(const std::string& prompt, T low, T high) {
    std::istringstream in(line(prompt));
    T value{};
    if (!(in >> value)) throw std::invalid_argument("Ожидалось число");
    in >> std::ws;
    if (!in.eof() || value < low || value > high)
        throw std::invalid_argument("Число вне допустимого диапазона или лишние символы");
    if constexpr (std::is_floating_point<T>::value)
        if (!std::isfinite(value)) throw std::invalid_argument("Число должно быть конечным");
    return value;
}
inline int integer(const std::string& prompt) {
    return number<int>(prompt, -1000000000, 1000000000);
}
inline double real(const std::string& prompt) {
    return number<double>(prompt, -1e100, 1e100);
}
inline std::vector<int> integers(const std::string& prompt) {
    std::istringstream in(line(prompt));
    std::vector<int> result;
    std::string token;
    while (in >> token) {
        std::istringstream item(token);
        long long value;
        if (!(item >> value) || !item.eof())
            throw std::invalid_argument("Ожидался список целых чисел");
        if (value < -1000000000 || value > 1000000000)
            throw std::invalid_argument("Элемент должен быть от -10^9 до 10^9");
        result.push_back(static_cast<int>(value));
    }
    return result;
}
template<class Collection>
void print_values(const Collection& collection) {
    std::cout << "{ ";
    std::size_t shown = 0;
    collection.for_each([&](const auto& value) {
        if (shown++ < 100) std::cout << value << ' ';
    });
    if (shown > 100) std::cout << "... ";
    std::cout << "} (" << shown << ")\n";
}
} // namespace cpp_labs
