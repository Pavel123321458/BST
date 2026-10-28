#include "test_support.hpp"
#include "../lab1/tree_set.hpp"
#include <numeric>
#include <random>
#include <set>
#include <vector>

using namespace cpp_labs;
template<class Collection> std::vector<int> values(const Collection& c) {
    std::vector<int> result;
    c.for_each([&](int v) { result.push_back(v); });
    return result;
}
AvlTree<int> example_tree() {
    AvlTree<int> tree;
    for (int x : {4, 2, 6, 1, 3, 5, 7}) tree.insert(x);
    return tree;
}
TreeSet<int> make_set(std::initializer_list<int> items) {
    TreeSet<int> result;
    for (int x : items) result.add(x);
    return result;
}

int main() {
    TestRunner runner;
    runner.run("empty, duplicates and missing keys", [] {
        AvlTree<int> tree;
        CHECK(tree.empty()); CHECK(tree.height() == 0);
        CHECK(!tree.erase(5)); CHECK(!tree.contains(5)); CHECK(tree.serialize() == "#");
        CHECK(tree.insert(5)); CHECK(!tree.insert(5)); CHECK(tree.size() == 1);
        CHECK(tree.erase(5)); CHECK(tree.empty()); tree.validate();
        expect_throw<std::out_of_range>([&] { tree.subtree(7); });
    });
    runner.run("all four rotation cases", [] {
        for (const auto& order : std::vector<std::vector<int>>{{3,2,1},{1,2,3},{3,1,2},{1,3,2}}) {
            AvlTree<int> tree;
            for (int x : order) { tree.insert(x); tree.validate(); }
            CHECK(tree.height() == 2);
            CHECK(tree.serialize() == "( 2 ( 1 # # ) ( 3 # # ) )");
        }
    });
    runner.run("six traversal orders", [] {
        auto tree = example_tree();
        const Order orders[] = {Order::KLP, Order::KPL, Order::LKP, Order::LPK, Order::PKL, Order::PLK};
        const std::vector<std::vector<int>> expected = {
            {4,2,1,3,6,5,7}, {4,6,7,5,2,3,1}, {1,2,3,4,5,6,7},
            {1,3,2,5,7,6,4}, {7,6,5,4,3,2,1}, {7,5,6,3,1,2,4}};
        for (int i = 0; i < 6; ++i) {
            std::vector<int> result;
            tree.for_each([&](int x) { result.push_back(x); }, orders[i]);
            CHECK(result == expected[i]);
        }
    });
    runner.run("deletion of root, leaf, one-child and two-child nodes", [] {
        auto tree = example_tree();
        for (int x : {1,2,4,6,3,5,7}) { CHECK(tree.erase(x)); tree.validate(); CHECK(!tree.contains(x)); }
        CHECK(tree.empty());
    });
    runner.run("random operations compared with std::set", [] {
        std::mt19937 random(12345);
        AvlTree<int> tree;
        std::set<int> reference;
        for (int step = 0; step < 15000; ++step) {
            int x = static_cast<int>(random() % 501) - 250;
            switch (random() % 3) {
                case 0: CHECK(tree.insert(x) == reference.insert(x).second); break;
                case 1: CHECK(tree.erase(x) == (reference.erase(x) != 0)); break;
                default: CHECK(tree.contains(x) == (reference.count(x) != 0));
            }
            tree.validate();
            CHECK(tree.size() == reference.size());
            if (step % 100 == 0) CHECK(values(tree) == std::vector<int>(reference.begin(), reference.end()));
        }
    });
    runner.run("ordered input remains balanced", [] {
        AvlTree<int> tree;
        for (int i = 0; i < 20000; ++i) tree.insert(i);
        tree.validate(); CHECK(tree.height() < 22);
        for (int i = 19999; i >= 0; --i) { CHECK(tree.erase(i)); if (i % 100 == 0) tree.validate(); }
        CHECK(tree.empty());
    });
    runner.run("deep copy, assignment, self assignment and move", [] {
        auto tree = example_tree();
        auto copy = tree;
        copy.erase(4); CHECK(tree.contains(4));
        copy = tree; copy = copy; CHECK(values(copy) == values(tree));
        AvlTree<int> moved(std::move(copy));
        CHECK(copy.empty()); copy.validate(); CHECK(moved.size() == 7);
        moved.clear(); CHECK(moved.empty()); CHECK(tree.size() == 7);
    });
    runner.run("map, where and reduce including empty input and collisions", [] {
        auto tree = example_tree();
        auto mapped = tree.map([](int x) { return -x; }); mapped.validate();
        CHECK(values(mapped) == std::vector<int>({-7,-6,-5,-4,-3,-2,-1}));
        CHECK(tree.map([](int) { return 0; }).size() == 1);
        CHECK(values(tree.where([](int x) { return x % 2 == 0; })) == std::vector<int>({2,4,6}));
        CHECK(tree.where([](int) { return false; }).empty());
        CHECK(tree.reduce(10, [](int a, int b) { return a + b; }) == 38);
        CHECK(AvlTree<int>().reduce(10, [](int a, int b) { return a + b; }) == 10);
        CHECK(tree.size() == 7);
    });
    runner.run("subtree extraction and structural containment", [] {
        auto tree = example_tree();
        auto part = tree.subtree(2);
        CHECK(values(part) == std::vector<int>({1,2,3})); CHECK(tree.contains_subtree(part));
        CHECK(tree.contains_subtree(tree)); CHECK(tree.contains_subtree(AvlTree<int>()));
        part.erase(1); CHECK(!tree.contains_subtree(part)); CHECK(tree.contains(1));
        CHECK(!AvlTree<int>().contains_subtree(tree));
        expect_throw<std::out_of_range>([&] { tree.subtree(50); });
    });
    runner.run("serialization preserves values and shape", [] {
        auto tree = example_tree();
        auto restored = AvlTree<int>::deserialize(tree.serialize());
        CHECK(restored.serialize() == tree.serialize());
        CHECK(AvlTree<int>::deserialize(" # \n").empty());
        AvlTree<double> reals; reals.insert(0.1); reals.insert(-1.2345678901234567);
        CHECK(AvlTree<double>::deserialize(reals.serialize()).serialize() == reals.serialize());
        AvlTree<std::string> strings;
        for (std::string x : {"hello world", "", "a\"b\\c", "# ( )"}) strings.insert(x);
        CHECK(AvlTree<std::string>::deserialize(strings.serialize()).serialize() == strings.serialize());
    });
    runner.run("invalid serialized input is rejected", [] {
        for (const std::string text : {"", "(", "( x # # )", "# trailing", "( 1 # #", "( 2 ( 3 # # ) # )",
             "( 2 ( 2 # # ) # )", "( 1 # ( 2 # ( 3 # # ) ) )", "( 2147483648 # # )"}) {
            expect_throw<std::invalid_argument>([&] { AvlTree<int>::deserialize(text); });
        }
    });
    runner.run("set operations, subset, equality and serialization", [] {
        auto a = make_set({1,2,3,3}), b = make_set({3,4});
        CHECK(a.size() == 3); CHECK(!a.add(1)); CHECK(!a.remove(9));
        CHECK(values(a.unite(b)) == std::vector<int>({1,2,3,4}));
        CHECK(values(a.intersect(b)) == std::vector<int>({3}));
        CHECK(values(a.difference(b)) == std::vector<int>({1,2}));
        CHECK(a.intersect(b).is_subset_of(a)); CHECK(!a.is_subset_of(b));
        CHECK(a.equals(make_set({3,2,1}))); CHECK(!a.equals(b));
        CHECK(TreeSet<int>().is_subset_of(a));
        CHECK(TreeSet<int>::deserialize(a.serialize()).equals(a));
        CHECK(a.map([](int x) { return x % 2; }).equals(make_set({0,1})));
        CHECK(a.where([](int x) { return x > 1; }).equals(make_set({2,3})));
        CHECK(a.reduce(0, [](int s, int x) { return s + x; }) == 6);
        CHECK(a.remove(2)); CHECK(!a.contains(2));
    });
    return runner.finish();
}
