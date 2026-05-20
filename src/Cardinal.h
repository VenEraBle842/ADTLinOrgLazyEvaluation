#pragma once
#include <stdexcept>

// Структура для представления мощности множества и ординалов формы: omega * infiniteCount + value.
struct Cardinal {
    bool isInfinite;
    size_t infiniteCount; // Количество бесконечностей (множитель перед omega)
    size_t value;         // Конечный остаток

    explicit Cardinal(size_t val) : isInfinite(false), infiniteCount(0), value(val) {}
    Cardinal() : isInfinite(true), infiniteCount(1), value(0) {} // По умолчанию 1 * omega
    Cardinal(size_t infCnt, size_t val) : isInfinite(infCnt > 0), infiniteCount(infCnt), value(val) {}

    static Cardinal Infinity() {
        return {1, 0};
    }

    bool operator==(const Cardinal& other) const {
        return infiniteCount == other.infiniteCount && value == other.value;
    }

    bool operator!=(const Cardinal& other) const {
        return !(*this == other);
    }

    bool operator<(const Cardinal& other) const {
        if (infiniteCount < other.infiniteCount) return true;
        if (infiniteCount > other.infiniteCount) return false;
        return value < other.value;
    }

    Cardinal operator+(const Cardinal& other) const {
        if (other.isInfinite) {
            // omega * c1 + v1 + omega * c2 + v2 = omega * (c1 + c2) + v2
            // Конечное слагаемое первого ординала поглощается второй бесконечностью
            return {infiniteCount + other.infiniteCount, other.value};
        }
        return {infiniteCount, value + other.value};
    }

    Cardinal operator-(const Cardinal& other) const {
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
