#pragma once
#include "dictionary.hpp"
#include <cmath>
#include <fstream>
#include <limits>

namespace cpp_labs {

class SparseVector {
    std::size_t dimension_;
    Backend backend_;
    std::unique_ptr<IDictionary<std::size_t, double>> data_;
    void check_index(std::size_t index) const {
        if (index >= dimension_) throw std::out_of_range("Index is outside vector dimension");
    }
    void check_dimension(const SparseVector& other) const {
        if (dimension_ != other.dimension_) throw std::invalid_argument("Different vector dimensions");
    }
public:
    explicit SparseVector(std::size_t dimension = 0, Backend backend = Backend::Avl)
        : dimension_(dimension), backend_(backend), data_(make_dictionary<std::size_t, double>(backend)) {}
    SparseVector(const SparseVector& other) : SparseVector(other.dimension_, other.backend_) {
        other.for_each([&](std::size_t i, double value) { data_->Add(i, value); });
    }
    SparseVector(SparseVector&&) noexcept = default;
    SparseVector& operator=(SparseVector other) {
        using std::swap;
        swap(dimension_, other.dimension_); swap(backend_, other.backend_); swap(data_, other.data_);
        return *this;
    }
    std::size_t dimension() const { return dimension_; }
    std::size_t nonzero_count() const { return data_->GetCount(); }
    Backend backend() const { return backend_; }
    double get(std::size_t index) const {
        check_index(index);
        return data_->ContainsKey(index) ? data_->Get(index) : 0.0;
    }
    void set(std::size_t index, double value) {
        check_index(index);
        if (!std::isfinite(value)) throw std::invalid_argument("Value must be finite");
        if (value == 0.0) {
            if (data_->ContainsKey(index)) data_->Remove(index);
        } else data_->Set(index, value);
    }
    template<class F> void for_each(F visit) const { data_->ForEach(visit); }
    SparseVector converted(Backend backend) const {
        SparseVector result(dimension_, backend);
        for_each([&](std::size_t i, double value) { result.set(i, value); });
        return result;
    }
    SparseVector add(const SparseVector& other) const {
        check_dimension(other);
        SparseVector result(*this);
        other.for_each([&](std::size_t i, double value) { result.set(i, result.get(i) + value); });
        return result;
    }
    SparseVector scaled(double factor) const {
        if (!std::isfinite(factor)) throw std::invalid_argument("Factor must be finite");
        SparseVector result(dimension_, backend_);
        for_each([&](std::size_t i, double value) { result.set(i, value * factor); });
        return result;
    }
    double dot(const SparseVector& other) const {
        check_dimension(other);
        double result = 0;
        for_each([&](std::size_t i, double value) { result += value * other.get(i); });
        if (!std::isfinite(result)) throw std::overflow_error("Dot product overflow");
        return result;
    }
    void save(std::ostream& out) const {
        out << "SPARSE_VECTOR 1\n" << dimension_ << ' ' << nonzero_count() << '\n';
        out << std::setprecision(std::numeric_limits<double>::max_digits10);
        for_each([&](std::size_t i, double value) { out << i << ' ' << value << '\n'; });
        if (!out) throw std::runtime_error("Cannot write vector");
    }
    static SparseVector load(std::istream& in, Backend backend) {
        std::string magic;
        int version;
        long long dimension, count;
        if (!(in >> magic >> version) || magic != "SPARSE_VECTOR" || version != 1)
            throw std::invalid_argument("Expected SPARSE_VECTOR 1 header");
        if (!(in >> dimension >> count) || dimension < 0 || count < 0 || count > dimension)
            throw std::invalid_argument("Invalid dimension or element count");
        SparseVector result(static_cast<std::size_t>(dimension), backend);
        for (long long n = 0; n < count; ++n) {
            long long index;
            double value;
            if (!(in >> index >> value) || index < 0 || index >= dimension || value == 0 || !std::isfinite(value))
                throw std::invalid_argument("Invalid nonzero entry");
            if (result.data_->ContainsKey(static_cast<std::size_t>(index)))
                throw std::invalid_argument("Duplicate vector index");
            result.set(static_cast<std::size_t>(index), value);
        }
        in >> std::ws;
        if (!in.eof()) throw std::invalid_argument("Extra text after vector");
        return result;
    }
    void save_file(const std::string& path) const {
        std::ofstream out(path);
        if (!out) throw std::runtime_error("Cannot open output file");
        save(out);
    }
    static SparseVector load_file(const std::string& path, Backend backend) {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open input file");
        return load(in, backend);
    }
};

} // namespace cpp_labs
