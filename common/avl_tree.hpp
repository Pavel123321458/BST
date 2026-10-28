#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace cpp_labs {

enum class Order { KLP, KPL, LKP, LPK, PKL, PLK };

// Шаблонное AVL-дерево: каждый ключ хранится один раз.
// unique_ptr владеет узлом и освобождает его вместе с потомками.
template<class T, class Compare = std::less<T>>
class AvlTree {
    struct Node {
        T value;
        std::unique_ptr<Node> left, right;
        int height = 1;
        explicit Node(const T& value) : value(value) {}
    };
    using Link = std::unique_ptr<Node>;
    Link root_;
    std::size_t size_ = 0;
    Compare compare_;

    static int height(const Link& p) { return p ? p->height : 0; }
    static void refresh(Node& p) {
        p.height = 1 + std::max(height(p.left), height(p.right));
    }
    static void rotate_left(Link& p) {
        Link next = std::move(p->right);
        p->right = std::move(next->left);
        refresh(*p);
        next->left = std::move(p);
        refresh(*next);
        p = std::move(next);
    }
    static void rotate_right(Link& p) {
        Link next = std::move(p->left);
        p->left = std::move(next->right);
        refresh(*p);
        next->right = std::move(p);
        refresh(*next);
        p = std::move(next);
    }
    static void balance(Link& p) {
        if (!p) return;
        refresh(*p);
        const int factor = height(p->right) - height(p->left);
        if (factor > 1) {
            if (height(p->right->left) > height(p->right->right))
                rotate_right(p->right);
            rotate_left(p);
        } else if (factor < -1) {
            if (height(p->left->right) > height(p->left->left))
                rotate_left(p->left);
            rotate_right(p);
        }
    }
    bool insert_node(Link& p, const T& value) {
        if (!p) {
            p = std::make_unique<Node>(value);
            ++size_;
            return true;
        }
        bool added;
        if (compare_(value, p->value)) added = insert_node(p->left, value);
        else if (compare_(p->value, value)) added = insert_node(p->right, value);
        else return false;
        balance(p);
        return added;
    }
    static Link detach_min(Link& p) {
        if (!p->left) {
            Link result = std::move(p);
            p = std::move(result->right);
            return result;
        }
        Link result = detach_min(p->left);
        balance(p);
        return result;
    }
    bool erase_node(Link& p, const T& value) {
        if (!p) return false;
        bool removed = true;
        if (compare_(value, p->value)) removed = erase_node(p->left, value);
        else if (compare_(p->value, value)) removed = erase_node(p->right, value);
        else {
            Link old = std::move(p);
            if (!old->left) p = std::move(old->right);
            else if (!old->right) p = std::move(old->left);
            else {
                p = detach_min(old->right);
                p->left = std::move(old->left);
                p->right = std::move(old->right);
            }
            --size_;
        }
        balance(p);
        return removed;
    }
    const Node* find_node(const T& value) const {
        const Node* p = root_.get();
        while (p) {
            if (compare_(value, p->value)) p = p->left.get();
            else if (compare_(p->value, value)) p = p->right.get();
            else return p;
        }
        return nullptr;
    }
    static Link clone(const Node* p) {
        if (!p) return nullptr;
        Link result = std::make_unique<Node>(p->value);
        result->left = clone(p->left.get());
        result->right = clone(p->right.get());
        result->height = p->height;
        return result;
    }
    static std::size_t count(const Node* p) {
        return p ? 1 + count(p->left.get()) + count(p->right.get()) : 0;
    }
    template<class F>
    static void walk(const Node* p, Order order, F& visit) {
        if (!p) return;
        const bool reverse = order == Order::KPL || order == Order::PKL || order == Order::PLK;
        const Node* first = reverse ? p->right.get() : p->left.get();
        const Node* second = reverse ? p->left.get() : p->right.get();
        if (order == Order::KLP || order == Order::KPL) visit(p->value);
        walk(first, order, visit);
        if (order == Order::LKP || order == Order::PKL) visit(p->value);
        walk(second, order, visit);
        if (order == Order::LPK || order == Order::PLK) visit(p->value);
    }
    static bool same(const Node* a, const Node* b, const Compare& compare) {
        if (!a || !b) return a == b;
        return !compare(a->value, b->value) && !compare(b->value, a->value)
            && same(a->left.get(), b->left.get(), compare)
            && same(a->right.get(), b->right.get(), compare);
    }
    static void write_value(std::ostream& out, const T& value) { out << value; }
    static void read_value(std::istream& in, T& value) { in >> value; }
    // Строки могут содержать пробелы, скобки и кавычки.
    template<class U = T>
    static std::enable_if_t<std::is_same<U, std::string>::value>
    write_string(std::ostream& out, const U& value) { out << std::quoted(value); }
    static void save_node(std::ostream& out, const Node* p) {
        if (!p) { out << '#'; return; }
        out << "( ";
        if constexpr (std::is_same<T, std::string>::value) write_string(out, p->value);
        else write_value(out, p->value);
        out << ' ';
        save_node(out, p->left.get());
        out << ' ';
        save_node(out, p->right.get());
        out << " )";
    }
    static Link load_node(std::istream& in, std::size_t& count, int depth) {
        if (depth > 64) throw std::invalid_argument("Tree nesting is too deep");
        char token;
        if (!(in >> token)) throw std::invalid_argument("Unexpected end of tree");
        if (token == '#') return nullptr;
        if (token != '(') throw std::invalid_argument("Expected '(' or '#'");
        T value{};
        if constexpr (std::is_same<T, std::string>::value) in >> std::quoted(value);
        else read_value(in, value);
        if (!in) throw std::invalid_argument("Invalid node value");
        Link p = std::make_unique<Node>(value);
        ++count;
        p->left = load_node(in, count, depth + 1);
        p->right = load_node(in, count, depth + 1);
        if (!(in >> token) || token != ')') throw std::invalid_argument("Expected ')'");
        refresh(*p);
        return p;
    }
    int check_node(const Node* p, const T* low, const T* high, std::size_t& count) const {
        if (!p) return 0;
        if ((low && !compare_(*low, p->value)) || (high && !compare_(p->value, *high)))
            throw std::logic_error("Invalid search-tree ordering");
        int left = check_node(p->left.get(), low, &p->value, count);
        int right = check_node(p->right.get(), &p->value, high, count);
        if (std::abs(left - right) > 1 || p->height != 1 + std::max(left, right))
            throw std::logic_error("Invalid AVL balance or height");
        ++count;
        return p->height;
    }

public:
    explicit AvlTree(Compare compare = Compare{}) : compare_(std::move(compare)) {}
    AvlTree(const AvlTree& other)
        : root_(clone(other.root_.get())), size_(other.size_), compare_(other.compare_) {}
    AvlTree(AvlTree&& other) noexcept
        : root_(std::move(other.root_)), size_(std::exchange(other.size_, 0)),
          compare_(std::move(other.compare_)) {}
    AvlTree& operator=(AvlTree other) { swap(other); return *this; }
    void swap(AvlTree& other) noexcept {
        using std::swap;
        swap(root_, other.root_); swap(size_, other.size_); swap(compare_, other.compare_);
    }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int height() const { return height(root_); }
    void clear() { root_.reset(); size_ = 0; }
    bool insert(const T& value) { return insert_node(root_, value); }
    bool erase(const T& value) { return erase_node(root_, value); }
    const T* find(const T& value) const {
        const Node* p = find_node(value);
        return p ? &p->value : nullptr;
    }
    bool contains(const T& value) const { return find(value) != nullptr; }
    template<class F>
    void for_each(F visit, Order order = Order::LKP) const { walk(root_.get(), order, visit); }
    template<class F>
    AvlTree map(F transform) const {
        AvlTree result(compare_);
        for_each([&](const T& value) { result.insert(transform(value)); });
        return result;
    }
    template<class Predicate>
    AvlTree where(Predicate predicate) const {
        AvlTree result(compare_);
        for_each([&](const T& value) { if (predicate(value)) result.insert(value); });
        return result;
    }
    template<class Accumulator, class F>
    Accumulator reduce(Accumulator initial, F combine) const {
        for_each([&](const T& value) { initial = combine(initial, value); });
        return initial;
    }
    AvlTree subtree(const T& root) const {
        const Node* p = find_node(root);
        if (!p) throw std::out_of_range("Subtree root not found");
        AvlTree result(compare_);
        result.root_ = clone(p);
        result.size_ = count(p);
        return result;
    }
    // Совпадать должны и значения, и форма всего поддерева.
    bool contains_subtree(const AvlTree& other) const {
        return other.empty() || same(find_node(other.root_->value), other.root_.get(), compare_);
    }
    std::string serialize() const {
        std::ostringstream out;
        out << std::setprecision(17);
        save_node(out, root_.get());
        return out.str();
    }
    static AvlTree deserialize(const std::string& text, Compare compare = Compare{}) {
        AvlTree result(compare);
        std::istringstream in(text);
        result.root_ = load_node(in, result.size_, 0);
        in >> std::ws;
        if (!in.eof()) throw std::invalid_argument("Extra text after tree");
        try { result.validate(); }
        catch (const std::logic_error& e) { throw std::invalid_argument(e.what()); }
        return result;
    }
    void validate() const {
        std::size_t count = 0;
        check_node(root_.get(), nullptr, nullptr, count);
        if (count != size_) throw std::logic_error("Invalid tree size");
    }
};

} // namespace cpp_labs
