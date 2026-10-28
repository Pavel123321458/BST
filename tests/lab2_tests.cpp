#include "test_support.hpp"
#include "../lab2/sparse_vector.hpp"
#include <map>
#include <random>

using namespace cpp_labs;

void dictionary_test(Backend backend) {
    auto dictionary = make_dictionary<int, int>(backend);
    std::map<int, int> reference;
    std::mt19937 random(9876);
    expect_throw<std::out_of_range>([&] { dictionary->Get(0); });
    expect_throw<std::out_of_range>([&] { dictionary->Remove(0); });
    dictionary->Add(10, 20);
    expect_throw<std::invalid_argument>([&] { dictionary->Add(10, 30); });
    CHECK(dictionary->Get(10) == 20);
    dictionary->Remove(10);
    for (int step = 0; step < 5000; ++step) {
        int key = static_cast<int>(random() % 101) - 50;
        int value = static_cast<int>(random() % 1000);
        if (random() % 3 == 0) {
            if (reference.erase(key)) dictionary->Remove(key);
            else expect_throw<std::out_of_range>([&] { dictionary->Remove(key); });
        } else { dictionary->Set(key, value); reference[key] = value; }
        CHECK(dictionary->GetCount() == reference.size());
        CHECK(dictionary->GetCapacity() >= dictionary->GetCount());
        for (int candidate : {-51, key, 51}) {
            CHECK(dictionary->ContainsKey(candidate) == (reference.count(candidate) != 0));
            if (reference.count(candidate)) CHECK(dictionary->Get(candidate) == reference.at(candidate));
        }
        if (step % 100 == 0) {
            std::map<int, int> observed;
            dictionary->ForEach([&](int k, int v) { observed[k] = v; });
            CHECK(observed == reference);
        }
    }
}
int main() {
    TestRunner runner;
    runner.run("AVL dictionary against std::map", [] { dictionary_test(Backend::Avl); });
    runner.run("binary-search dictionary against std::map", [] { dictionary_test(Backend::Sorted); });
    runner.run("dictionary generic key/value types", [] {
        for (Backend backend : {Backend::Avl, Backend::Sorted}) {
            auto d = make_dictionary<std::string, std::string>(backend);
            d->Add("name", "hello world"); d->Add("", "empty key");
            CHECK(d->Get("name") == "hello world"); CHECK(d->Get("") == "empty key");
        }
    });
    runner.run("sparse access, zero removal, boundaries and non-finite values", [] {
        for (Backend backend : {Backend::Avl, Backend::Sorted}) {
            SparseVector v(1000000000, backend);
            CHECK(v.get(0) == 0); v.set(999999999, 7); CHECK(v.nonzero_count() == 1);
            CHECK(v.get(999999999) == 7); v.set(999999999, 8); CHECK(v.nonzero_count() == 1);
            v.set(999999999, 0); CHECK(v.nonzero_count() == 0);
            expect_throw<std::out_of_range>([&] { v.set(1000000000, 1); });
            expect_throw<std::out_of_range>([&] { v.get(1000000000); });
            expect_throw<std::invalid_argument>([&] { v.set(0, std::numeric_limits<double>::infinity()); });
            expect_throw<std::invalid_argument>([&] { v.set(0, std::numeric_limits<double>::quiet_NaN()); });
            SparseVector empty;
            expect_throw<std::out_of_range>([&] { empty.get(0); });
        }
    });
    runner.run("sparse operations compared with dense reference", [] {
        std::mt19937 random(555);
        for (Backend first : {Backend::Avl, Backend::Sorted}) {
            for (Backend second : {Backend::Avl, Backend::Sorted}) {
                SparseVector a(100, first), b(100, second);
                std::vector<double> ar(100), br(100);
                for (int step = 0; step < 500; ++step) {
                    std::size_t i = random() % 100;
                    double value = static_cast<int>(random() % 21) - 10;
                    a.set(i, value); ar[i] = value;
                    i = random() % 100; value = static_cast<int>(random() % 21) - 10;
                    b.set(i, value); br[i] = value;
                }
                auto sum = a.add(b), scaled = a.scaled(-2);
                double dot = 0;
                for (std::size_t i = 0; i < 100; ++i) {
                    CHECK(a.get(i) == ar[i]); CHECK(b.get(i) == br[i]);
                    CHECK(sum.get(i) == ar[i] + br[i]); CHECK(scaled.get(i) == -2 * ar[i]);
                    dot += ar[i] * br[i];
                }
                CHECK(a.dot(b) == dot); CHECK(a.scaled(0).nonzero_count() == 0);
            }
        }
    });
    runner.run("dimension mismatch and numerical overflow", [] {
        SparseVector a(2), b(3);
        expect_throw<std::invalid_argument>([&] { a.add(b); });
        expect_throw<std::invalid_argument>([&] { a.dot(b); });
        a.set(0, std::numeric_limits<double>::max());
        expect_throw<std::invalid_argument>([&] { a.scaled(2); });
        expect_throw<std::overflow_error>([&] { a.dot(a); });
        CHECK(SparseVector().dot(SparseVector()) == 0);
    });
    runner.run("copy independence and backend conversion", [] {
        SparseVector a(20); a.set(5, 3);
        auto b = a; b.set(5, 9); CHECK(a.get(5) == 3);
        b = a; CHECK(b.get(5) == 3);
        auto c = a.converted(Backend::Sorted); CHECK(c.get(5) == 3);
        c.set(5, 0); CHECK(a.get(5) == 3);
    });
    runner.run("file format round trip across backends", [] {
        SparseVector a(10000000); a.set(0, 0.1); a.set(9999999, -1.2345678901234567);
        std::ostringstream out; a.save(out);
        for (Backend backend : {Backend::Avl, Backend::Sorted}) {
            std::istringstream in(out.str());
            auto b = SparseVector::load(in, backend);
            CHECK(b.dimension() == a.dimension()); CHECK(b.nonzero_count() == 2);
            CHECK(b.get(0) == a.get(0)); CHECK(b.get(9999999) == a.get(9999999));
        }
        std::istringstream empty("SPARSE_VECTOR 1\n0 0\n");
        CHECK(SparseVector::load(empty, Backend::Avl).dimension() == 0);
    });
    runner.run("malformed files are rejected", [] {
        for (const std::string text : {"", "SPARSE_VECTOR 2\n1 0", "SPARSE_VECTOR 1\n-1 0",
             "SPARSE_VECTOR 1\n2 3", "SPARSE_VECTOR 1\n2 1\n2 5", "SPARSE_VECTOR 1\n2 1\n-1 5",
             "SPARSE_VECTOR 1\n2 1\n0 0", "SPARSE_VECTOR 1\n2 2\n0 1\n0 2",
             "SPARSE_VECTOR 1\n2 1", "SPARSE_VECTOR 1\n2 0\nextra", "SPARSE_VECTOR 1\n2 1\n0 nan"}) {
            for (Backend backend : {Backend::Avl, Backend::Sorted}) {
                expect_throw<std::invalid_argument>([&] {
                    std::istringstream in(text); SparseVector::load(in, backend);
                });
            }
        }
    });
    return runner.finish();
}
