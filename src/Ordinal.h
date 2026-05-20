#pragma once
#include <stdexcept>

// Структура для представления мощности множества и ординалов формы: omega * infiniteCount + value.
struct Ordinal {
    bool isInfinite;
    size_t infiniteCount; // Количество бесконечностей (множитель перед omega)
    size_t value;         // Конечный остаток

    explicit Ordinal(size_t val) : isInfinite(false), infiniteCount(0), value(val) {}
    Ordinal() : isInfinite(true), infiniteCount(1), value(0) {} // По умолчанию 1 * omega
    Ordinal(size_t infCnt, size_t val) : isInfinite(infCnt > 0), infiniteCount(infCnt), value(val) {}

    static Ordinal Infinity() {
        return {1, 0};
    }

    bool operator==(const Ordinal& other) const {
        return infiniteCount == other.infiniteCount && value == other.value;
    }

    bool operator!=(const Ordinal& other) const {
        return !(*this == other);
    }

    bool operator<(const Ordinal& other) const {
        if (infiniteCount < other.infiniteCount) return true;
        if (infiniteCount > other.infiniteCount) return false;
        return value < other.value;
    }

    Ordinal operator+(const Ordinal& other) const {
        if (other.isInfinite) {
            // omega * c1 + v1 + omega * c2 + v2 = omega * (c1 + c2) + v2
            // Конечное слагаемое первого ординала поглощается второй бесконечностью
            return {infiniteCount + other.infiniteCount, other.value};
        }
        return {infiniteCount, value + other.value};
    }

    Ordinal operator-(const Ordinal& other) const {
        if (other.infiniteCount > infiniteCount) {
            return {0, 0};
        }
        if (other.infiniteCount == infiniteCount) {
            if (isInfinite) {
                throw std::invalid_argument("Infinity minus infinity is undefined");
            }
            if (value > other.value) {
                return {0, value - other.value};
            }
            return {0, 0};
        }
        // Левое вычитание: a - b = c <=> b + c = a
        return {infiniteCount - other.infiniteCount, value};
    }
};
