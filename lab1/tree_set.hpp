#pragma once
#include "../common/avl_tree.hpp"

namespace cpp_labs {

template<class T>
class TreeSet {
    AvlTree<T> tree_;
public:
    bool add(const T& value) { return tree_.insert(value); }
    bool remove(const T& value) { return tree_.erase(value); }
    bool contains(const T& value) const { return tree_.contains(value); }
    std::size_t size() const { return tree_.size(); }
    template<class F> void for_each(F visit) const { tree_.for_each(visit); }
    template<class F> TreeSet map(F transform) const {
        TreeSet result; result.tree_ = tree_.map(transform); return result;
    }
    template<class P> TreeSet where(P predicate) const {
        TreeSet result; result.tree_ = tree_.where(predicate); return result;
    }
    template<class A, class F> A reduce(A initial, F combine) const {
        return tree_.reduce(initial, combine);
    }
    TreeSet unite(const TreeSet& other) const {
        TreeSet result = *this;
        other.for_each([&](const T& v) { result.add(v); });
        return result;
    }
    TreeSet intersect(const TreeSet& other) const {
        return where([&](const T& v) { return other.contains(v); });
    }
    TreeSet difference(const TreeSet& other) const {
        return where([&](const T& v) { return !other.contains(v); });
    }
    bool is_subset_of(const TreeSet& other) const {
        bool result = true;
        for_each([&](const T& v) { if (!other.contains(v)) result = false; });
        return result;
    }
    bool equals(const TreeSet& other) const {
        return size() == other.size() && is_subset_of(other);
    }
    std::string serialize() const { return tree_.serialize(); }
    static TreeSet deserialize(const std::string& text) {
        TreeSet result; result.tree_ = AvlTree<T>::deserialize(text); return result;
    }
};

} // namespace cpp_labs
