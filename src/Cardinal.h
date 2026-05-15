#pragma once
#include <stdexcept>

// Структура для представления мощности множества (количества элементов).
// Может быть как конечным натуральным числом, так и бесконечностью.
struct Cardinal {
    bool isInfinite;
    size_t value;

    explicit Cardinal(size_t val) : isInfinite(false), value(val) {}
    Cardinal() : isInfinite(true), value(0) {} // По умолчанию бесконечность

    static Cardinal Infinity() { return {}; }

    bool operator==(const Cardinal& other) const {
        if (isInfinite && other.isInfinite) return true;
        if (!isInfinite && !other.isInfinite) return value == other.value;
        return false;
    }

    bool operator!=(const Cardinal& other) const { return !(*this == other); }

    bool operator<(const Cardinal& other) const {
        if (isInfinite) return false;
        if (other.isInfinite) return true;
        return value < other.value;
    }

    Cardinal operator+(const Cardinal& other) const {
        if (isInfinite || other.isInfinite) return Infinity();
        return Cardinal(value + other.value);
    }

    Cardinal operator-(const Cardinal& other) const {
        if (other.isInfinite) {
            if (isInfinite) throw std::invalid_argument("Infinity minus infinity is undefined");
            return Cardinal(0); // Ограничим нулем (хотя технически это ошибка)
        }
        if (isInfinite) return Infinity();
        return value > other.value ? Cardinal(value - other.value) : Cardinal(0);
    }
};
