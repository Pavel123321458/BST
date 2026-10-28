#include "sparse_vector.hpp"
#include "../common/console.hpp"
#include <chrono>
#include <numeric>
#include <random>

using namespace cpp_labs;

Backend read_backend() {
    return number<int>("Структура: 1 AVL, 2 отсортированный массив: ", 1, 2) == 1
        ? Backend::Avl : Backend::Sorted;
}
void show(const SparseVector& vector) {
    std::cout << "Размерность " << vector.dimension() << ", ненулевых " << vector.nonzero_count()
        << ", структура " << (vector.backend() == Backend::Avl ? "AVL" : "Sorted") << '\n';
    std::size_t shown = 0;
    vector.for_each([&](std::size_t i, double x) {
        if (shown++ < 100) std::cout << '[' << i << "] = " << x << '\n';
    });
    if (shown > 100) std::cout << "... показаны первые 100 элементов\n";
    if (vector.dimension() <= 50) {
        std::cout << "Полный вид: ";
        for (std::size_t i = 0; i < vector.dimension(); ++i) std::cout << vector.get(i) << ' ';
        std::cout << '\n';
    }
}
SparseVector read_vector() {
    const auto dimension = number<long long>("Размерность (0..1000000000): ", 0, 1000000000);
    const Backend backend = read_backend();
    SparseVector result(static_cast<std::size_t>(dimension), backend);
    const auto count = number<long long>("Количество вводимых пар (0..размерность): ", 0, dimension);
    for (long long n = 0; n < count; ++n) {
        auto i = number<long long>("Индекс (начиная с 0): ", 0, dimension - 1);
        double value = real("Значение: ");
        result.set(static_cast<std::size_t>(i), value);
    }
    return result;
}
void compare_search(const SparseVector& vector) {
    auto first = vector.converted(Backend::Avl);
    auto second = vector.converted(Backend::Sorted);
    const auto index = number<long long>("Индекс для поиска: ", 0, static_cast<long long>(vector.dimension()) - 1);
    for (const SparseVector* current : {&first, &second}) {
        const auto start = std::chrono::steady_clock::now();
        double value = current->get(static_cast<std::size_t>(index));
        const auto stop = std::chrono::steady_clock::now();
        std::cout << (current->backend() == Backend::Avl ? "AVL" : "Binary search")
            << ": " << value << ", "
            << std::chrono::duration<double, std::micro>(stop - start).count() << " мкс\n";
    }
    std::cout << "Для устойчивого сравнения используйте задачу VS Code «Benchmark»: серии запросов и CSV.\n";
}

int main() {
    setup_console();
    SparseVector vector;
    try {
        while (true) {
            std::cout << "\nЛР 2 — РАЗРЕЖЕННЫЙ ВЕКТОР\n"
                "1 Ввести вектор   2 Задать элемент (0 удаляет)   3 Найти элемент\n"
                "4 Показать   5 Сменить структуру   6 Сложить с другим вектором\n"
                "7 Умножить на число   8 Скалярное произведение\n"
                "9 Сохранить в файл   10 Загрузить   11 Генерация\n"
                "12 Сравнить поиск   0 Выход\n";
            try {
                int choice = number<int>("> ", 0, 12);
                if (choice == 0) break;
                if (choice == 1) vector = read_vector();
                else if (choice == 2 || choice == 3) {
                    auto index = number<long long>("Индекс: ", 0, static_cast<long long>(vector.dimension()) - 1);
                    if (choice == 2) vector.set(static_cast<std::size_t>(index), real("Значение: "));
                    else std::cout << "Значение: " << vector.get(static_cast<std::size_t>(index)) << '\n';
                } else if (choice == 4) show(vector);
                else if (choice == 5) vector = vector.converted(read_backend());
                else if (choice == 6) {
                    std::cout << "Введите второй вектор\n";
                    auto other = read_vector();
                    auto result = vector.add(other);
                    std::cout << "Сумма (исходный вектор сохранён):\n"; show(result);
                } else if (choice == 7) show(vector.scaled(real("Множитель: ")));
                else if (choice == 8) {
                    std::cout << "Введите второй вектор\n";
                    auto other = read_vector();
                    std::cout << "Скалярное произведение: " << vector.dot(other) << '\n';
                } else if (choice == 9) {
                    vector.save_file(line("Путь к файлу (например, vector.txt): "));
                    std::cout << "Сохранено\n";
                } else if (choice == 10) {
                    auto path = line("Путь к файлу: ");
                    auto next = SparseVector::load_file(path, read_backend());
                    vector = std::move(next);
                    show(vector);
                } else if (choice == 11) {
                    auto dimension = number<long long>("Размерность (1..1000000000): ", 1, 1000000000);
                    auto count = number<long long>("Ненулевых (0..min(размерность,1000000)): ", 0,
                        std::min(dimension, 1000000LL));
                    int seed = number<int>("Seed: ", 0, 1000000000);
                    Backend backend = read_backend();
                    SparseVector next(static_cast<std::size_t>(dimension), backend);
                    std::mt19937 random(static_cast<unsigned>(seed));
                    // Равномерно распределяем индексы, значения генерируем случайно.
                    // Вставка по возрастанию быстра и для отсортированного массива.
                    for (long long n = 0; n < count; ++n)
                        next.set(static_cast<std::size_t>(n * dimension / count), 1 + random() % 99);
                    vector = std::move(next);
                    std::cout << "Создано ненулевых: " << vector.nonzero_count() << '\n';
                } else if (choice == 12) compare_search(vector);
            } catch (const std::exception& e) { std::cout << "Ошибка: " << e.what() << '\n'; }
        }
    } catch (const EndOfInput&) { std::cout << "\nКонец ввода\n"; }
    return 0;
}
