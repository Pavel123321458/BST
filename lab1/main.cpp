#include "tree_set.hpp"
#include "../common/console.hpp"
#include <random>

using namespace cpp_labs;

void set_menu() {
    TreeSet<int> a, b;
    while (true) {
        std::cout << "\nМНОЖЕСТВА A и B\n"
            "1 Ввести A   2 Ввести B   3 Показать\n"
            "4 Добавить в A   5 Удалить из A   6 Найти в A\n"
            "7 Объединение   8 Пересечение   9 Разность A-B\n"
            "10 Подмножество и равенство   11 map/where/reduce для A\n"
            "12 Сохранить A в строку   13 Прочитать A из строки   0 Назад\n";
        try {
            const int choice = number<int>("> ", 0, 13);
            if (choice == 0) return;
            if (choice == 1 || choice == 2) {
                auto values = integers("Элементы через пробел: ");
                TreeSet<int> result;
                for (int value : values) result.add(value);
                (choice == 1 ? a : b) = std::move(result);
            } else if (choice == 3) {
                std::cout << "A = "; print_values(a);
                std::cout << "B = "; print_values(b);
            } else if (choice == 4) {
                std::cout << (a.add(integer("Элемент: ")) ? "Добавлен\n" : "Уже есть\n");
            } else if (choice == 5) {
                std::cout << (a.remove(integer("Элемент: ")) ? "Удалён\n" : "Не найден\n");
            } else if (choice == 6) {
                std::cout << (a.contains(integer("Элемент: ")) ? "Найден\n" : "Не найден\n");
            } else if (choice == 7) print_values(a.unite(b));
            else if (choice == 8) print_values(a.intersect(b));
            else if (choice == 9) print_values(a.difference(b));
            else if (choice == 10) {
                std::cout << "A входит в B: " << (a.is_subset_of(b) ? "да" : "нет")
                    << "; A равно B: " << (a.equals(b) ? "да" : "нет") << '\n';
            } else if (choice == 11) {
                std::cout << "map(x/2): "; print_values(a.map([](int x) { return x / 2; }));
                std::cout << "where(x>0): "; print_values(a.where([](int x) { return x > 0; }));
                std::cout << "reduce(сумма): " << a.reduce(0LL, [](long long s, int x) { return s + x; }) << '\n';
            } else if (choice == 12) std::cout << a.serialize() << '\n';
            else if (choice == 13) a = TreeSet<int>::deserialize(line("Строка дерева: "));
        } catch (const std::exception& e) { std::cout << "Ошибка: " << e.what() << '\n'; }
    }
}

void demonstrate_types() {
    AvlTree<double> numbers;
    for (double value : {1.5, -2.25, 3.125}) numbers.insert(value);
    AvlTree<std::string> words;
    for (const std::string word : {"apple", "hello world", "pear"}) words.insert(word);
    std::cout << "Дерево double: "; print_values(numbers);
    std::cout << "Дерево string: "; print_values(words);
    std::cout << "Строковая сериализация: " << words.serialize() << '\n';
}

int main() {
    setup_console();
    AvlTree<int> tree;
    try {
        while (true) {
            std::cout << "\nЛР 1 — AVL-ДЕРЕВО\n"
                "1 Ввести дерево   2 Добавить   3 Удалить   4 Найти\n"
                "5 Обходы и высота   6 map/where/reduce   7 Извлечь поддерево\n"
                "8 Проверить вхождение поддерева   9 Сохранить в строку\n"
                "10 Прочитать из строки   11 Генерация   12 Множества\n"
                "13 Демонстрация других типов   0 Выход\n";
            try {
                const int choice = number<int>("> ", 0, 13);
                if (choice == 0) break;
                if (choice == 1) {
                    auto values = integers("Целые числа через пробел: ");
                    AvlTree<int> next;
                    for (int value : values) next.insert(value);
                    tree = std::move(next);
                } else if (choice == 2) {
                    std::cout << (tree.insert(integer("Элемент: ")) ? "Добавлен\n" : "Уже есть\n");
                } else if (choice == 3) {
                    std::cout << (tree.erase(integer("Элемент: ")) ? "Удалён\n" : "Не найден\n");
                } else if (choice == 4) {
                    std::cout << (tree.contains(integer("Элемент: ")) ? "Найден\n" : "Не найден\n");
                } else if (choice == 5) {
                    const char* names[] = {"КЛП", "КПЛ", "ЛКП", "ЛПК", "ПКЛ", "ПЛК"};
                    const Order orders[] = {Order::KLP, Order::KPL, Order::LKP, Order::LPK, Order::PKL, Order::PLK};
                    for (int i = 0; i < 6; ++i) {
                        std::cout << names[i] << ": ";
                        std::size_t shown = 0;
                        tree.for_each([&](int x) { if (shown++ < 100) std::cout << x << ' '; }, orders[i]);
                        if (shown > 100) std::cout << "...";
                        std::cout << '\n';
                    }
                    std::cout << "Узлов: " << tree.size() << "; высота: " << tree.height() << '\n';
                    tree.validate();
                    std::cout << "Порядок ключей и AVL-баланс корректны\n";
                } else if (choice == 6) {
                    std::cout << "map(x/2): "; print_values(tree.map([](int x) { return x / 2; }));
                    std::cout << "where(x>0): "; print_values(tree.where([](int x) { return x > 0; }));
                    std::cout << "reduce(сумма): " << tree.reduce(0LL, [](long long s, int x) { return s + x; }) << '\n';
                } else if (choice == 7) {
                    auto result = tree.subtree(integer("Корень поддерева: "));
                    std::cout << result.serialize() << '\n';
                } else if (choice == 8) {
                    auto part = AvlTree<int>::deserialize(line("Поддерево в формате КЛП (например, ( 2 # # )): "));
                    std::cout << (tree.contains_subtree(part) ? "Входит\n" : "Не входит\n");
                } else if (choice == 9) std::cout << tree.serialize() << '\n';
                else if (choice == 10) tree = AvlTree<int>::deserialize(line("Строка дерева: "));
                else if (choice == 11) {
                    const int count = number<int>("Число попыток вставки (0..1000000): ", 0, 1000000);
                    const int seed = number<int>("Seed (0..1000000000): ", 0, 1000000000);
                    const int limit = number<int>("Модуль границы значений (1..1000000000): ", 1, 1000000000);
                    std::mt19937 random(static_cast<unsigned>(seed));
                    std::uniform_int_distribution<int> values(-limit, limit);
                    AvlTree<int> next;
                    for (int i = 0; i < count; ++i) next.insert(values(random));
                    tree = std::move(next);
                    std::cout << "Создано уникальных узлов: " << tree.size() << '\n';
                } else if (choice == 12) set_menu();
                else if (choice == 13) demonstrate_types();
            } catch (const std::exception& e) { std::cout << "Ошибка: " << e.what() << '\n'; }
        }
    } catch (const EndOfInput&) { std::cout << "\nКонец ввода\n"; }
    return 0;
}
