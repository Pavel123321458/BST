#pragma once
#include "../common/avl_tree.hpp"
#include <vector>

namespace cpp_labs {

// Общий интерфейс позволяет менять способ поиска без изменения SparseVector.
template<class K, class V>
class IDictionary {
public:
    virtual ~IDictionary() = default;
    virtual std::size_t GetCount() const = 0;
    virtual std::size_t GetCapacity() const = 0;
    virtual const V& Get(const K& key) const = 0;
    virtual bool ContainsKey(const K& key) const = 0;
    virtual void Add(const K& key, const V& value) = 0;
    virtual void Set(const K& key, const V& value) = 0;
    virtual void Remove(const K& key) = 0;
    virtual void ForEach(const std::function<void(const K&, const V&)>& visit) const = 0;
};

template<class K, class V>
struct Entry {
    K key;
    V value;
    bool operator<(const Entry& other) const { return key < other.key; }
};

template<class K, class V>
class AvlDictionary : public IDictionary<K, V> {
    AvlTree<Entry<K, V>> tree_;
public:
    std::size_t GetCount() const override { return tree_.size(); }
    // Дерево выделяет память под каждый узел, свободного буфера у него нет.
    std::size_t GetCapacity() const override { return tree_.size(); }
    const V& Get(const K& key) const override {
        const auto* item = tree_.find({key, V{}});
        if (!item) throw std::out_of_range("Key not found");
        return item->value;
    }
    bool ContainsKey(const K& key) const override { return tree_.contains({key, V{}}); }
    void Add(const K& key, const V& value) override {
        if (!tree_.insert({key, value})) throw std::invalid_argument("Duplicate key");
    }
    void Set(const K& key, const V& value) override {
        tree_.erase({key, V{}});
        tree_.insert({key, value});
    }
    void Remove(const K& key) override {
        if (!tree_.erase({key, V{}})) throw std::out_of_range("Key not found");
    }
    void ForEach(const std::function<void(const K&, const V&)>& visit) const override {
        tree_.for_each([&](const Entry<K, V>& e) { visit(e.key, e.value); });
    }
    void Validate() const { tree_.validate(); }
};

template<class K, class V>
class SortedDictionary : public IDictionary<K, V> {
    std::vector<Entry<K, V>> items_;
    // Собственная реализация двоичного поиска первого ключа >= key.
    std::size_t lower_bound(const K& key) const {
        std::size_t left = 0, right = items_.size();
        while (left < right) {
            std::size_t middle = left + (right - left) / 2;
            if (items_[middle].key < key) left = middle + 1;
            else right = middle;
        }
        return left;
    }
    bool matches(std::size_t index, const K& key) const {
        return index < items_.size() && !(key < items_[index].key);
    }
public:
    std::size_t GetCount() const override { return items_.size(); }
    std::size_t GetCapacity() const override { return items_.capacity(); }
    const V& Get(const K& key) const override {
        const auto index = lower_bound(key);
        if (!matches(index, key)) throw std::out_of_range("Key not found");
        return items_[index].value;
    }
    bool ContainsKey(const K& key) const override { return matches(lower_bound(key), key); }
    void Add(const K& key, const V& value) override {
        const auto index = lower_bound(key);
        if (matches(index, key)) throw std::invalid_argument("Duplicate key");
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), {key, value});
    }
    void Set(const K& key, const V& value) override {
        const auto index = lower_bound(key);
        if (matches(index, key)) items_[index].value = value;
        else items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), {key, value});
    }
    void Remove(const K& key) override {
        const auto index = lower_bound(key);
        if (!matches(index, key)) throw std::out_of_range("Key not found");
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    }
    void ForEach(const std::function<void(const K&, const V&)>& visit) const override {
        for (const auto& item : items_) visit(item.key, item.value);
    }
};

enum class Backend { Avl, Sorted };

template<class K, class V>
std::unique_ptr<IDictionary<K, V>> make_dictionary(Backend backend) {
    if (backend == Backend::Avl) return std::make_unique<AvlDictionary<K, V>>();
    if (backend == Backend::Sorted) return std::make_unique<SortedDictionary<K, V>>();
    throw std::invalid_argument("Unknown dictionary backend");
}

} // namespace cpp_labs
