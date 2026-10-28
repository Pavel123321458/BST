#include "../common/avl_tree.hpp"
#include "../lab2/sparse_vector.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace cpp_labs;
using Clock = std::chrono::steady_clock;
template<class F> double milliseconds(F action) {
    const auto start = Clock::now();
    action();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
void tree_benchmark(std::size_t maximum, int repeats, const std::string& folder) {
    std::ofstream out(folder + "/lab1.csv");
    if (!out) throw std::runtime_error("Cannot write lab1.csv");
    out << "n,order,repeat,insert_ms,find_ms,erase_ms,checksum\n";
    for (std::size_t n = 10000; n <= maximum; n *= 10) {
        for (const std::string order : {"ascending", "descending", "random"}) {
            for (int repeat = 0; repeat < repeats; ++repeat) {
                std::vector<int> keys(n);
                std::iota(keys.begin(), keys.end(), 0);
                std::mt19937 random(42U + static_cast<unsigned>(repeat));
                if (order == "descending") std::reverse(keys.begin(), keys.end());
                if (order == "random") std::shuffle(keys.begin(), keys.end(), random);
                AvlTree<int> tree;
                const double build = milliseconds([&] { for (int key : keys) tree.insert(key); });
                tree.validate();
                std::size_t found = 0;
                const double search = milliseconds([&] {
                    for (int key : keys) found += tree.contains(key);
                });
                std::size_t removed = 0;
                const double erase = milliseconds([&] {
                    for (int key : keys) removed += tree.erase(key);
                });
                if (found != n || removed != n || !tree.empty()) throw std::logic_error("Tree benchmark failed");
                out << n << ',' << order << ',' << repeat + 1 << ',' << build << ',' << search << ',' << erase << ',' << found << '\n';
                std::cout << "Lab1 n=" << n << ' ' << order << " repeat=" << repeat + 1
                    << " build=" << build << " ms\n" << std::flush;
            }
        }
    }
    if (!out) throw std::runtime_error("Failed writing lab1.csv");
}
void sparse_benchmark(std::size_t maximum, int repeats, const std::string& folder) {
    std::ofstream out(folder + "/lab2.csv");
    if (!out) throw std::runtime_error("Cannot write lab2.csv");
    out << "n,dimension,order,backend,repeat,build_ms,search_ms,queries,checksum\n";
    std::vector<std::size_t> sizes;
    for (std::size_t n : {1000U, 5000U, 20000U}) if (n <= maximum) sizes.push_back(n);
    if (sizes.empty() || sizes.back() != maximum) sizes.push_back(maximum);
    for (std::size_t n : sizes) {
        for (const std::string order : {"ascending", "descending", "random"}) {
            for (int repeat = 0; repeat < repeats; ++repeat) {
                std::vector<std::size_t> keys(n);
                for (std::size_t i = 0; i < n; ++i) keys[i] = i * 10;
                std::mt19937 random(42U + static_cast<unsigned>(repeat));
                if (order == "descending") std::reverse(keys.begin(), keys.end());
                if (order == "random") std::shuffle(keys.begin(), keys.end(), random);
                std::vector<std::size_t> queries(100000);
                double expected = 0;
                for (auto& query : queries) {
                    query = random() % (n * 10);
                    if (query % 10 == 0) expected += 1 + (query / 10) % 97;
                }
                // Одинаковые ключи, значения и запросы для обоих алгоритмов.
                // Чередуем порядок запуска, уменьшая влияние прогрева процессора.
                for (int slot = 0; slot < 2; ++slot) {
                    const Backend backend = ((repeat + slot) % 2 == 0) ? Backend::Avl : Backend::Sorted;
                    SparseVector vector(n * 10, backend);
                    const double build = milliseconds([&] {
                        for (std::size_t key : keys) vector.set(key, 1 + (key / 10) % 97);
                    });
                    double checksum = 0;
                    const double search = milliseconds([&] {
                        for (std::size_t query : queries) checksum += vector.get(query);
                    });
                    if (checksum != expected || vector.nonzero_count() != n)
                        throw std::logic_error("Sparse benchmark failed");
                    const char* name = backend == Backend::Avl ? "AVL" : "BinarySearch";
                    out << n << ',' << n * 10 << ',' << order << ',' << name << ',' << repeat + 1 << ','
                        << build << ',' << search << ',' << queries.size() << ',' << checksum << '\n';
                    std::cout << "Lab2 n=" << n << ' ' << order << ' ' << name << " repeat=" << repeat + 1
                        << " build=" << build << " ms\n" << std::flush;
                }
            }
        }
    }
    if (!out) throw std::runtime_error("Failed writing lab2.csv");
}
int main(int argc, char** argv) {
    try {
        std::size_t tree_max = 1000000, sparse_max = 20000;
        int repeats = 3;
        std::string folder = "results";
        for (int i = 1; i < argc; ++i) {
            const std::string flag = argv[i];
            if (i + 1 == argc) throw std::invalid_argument("Missing argument value");
            const std::string value = argv[++i];
            if (flag == "--output") { folder = value; continue; }
            if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos)
                throw std::invalid_argument("Expected positive integer argument");
            const auto parsed = std::stoull(value);
            if (flag == "--tree-max" && parsed >= 10000 && parsed <= 100000000) tree_max = parsed;
            else if (flag == "--sparse-max" && parsed >= 1000 && parsed <= 100000) sparse_max = parsed;
            else if (flag == "--repeats" && parsed >= 1 && parsed <= 10) repeats = static_cast<int>(parsed);
            else throw std::invalid_argument("Unknown option or argument outside limits");
        }
        std::filesystem::create_directories(folder);
        tree_benchmark(tree_max, repeats, folder);
        sparse_benchmark(sparse_max, repeats, folder);
        std::cout << "CSV results saved to " << folder << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Benchmark error: " << e.what() << '\n';
        return 1;
    }
}
