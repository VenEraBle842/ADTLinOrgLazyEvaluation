#include <gtest/gtest.h>
#include "../src/LazySequence.h"
#include "ArraySequence.h" // Подтягиваем из твоего GitHub

// Правило генерации чисел Фибоначчи
int fibRule(Sequence<int>* window) {
    return window->Get(0) + window->Get(1);
}

TEST(LazySequenceTest, InfiniteGenerationAndMemoization) {
    Sequence<int>* initial = new MutableArraySequence<int>();
    appendTracked(initial, 0);
    appendTracked(initial, 1);

    LazySequence<int> fib(fibRule, initial, Cardinal::Infinity());

    EXPECT_TRUE(fib.GetCardinality().isInfinite);
    EXPECT_EQ(fib.GetMaterializedCount(), 0);

    // Доступ к 10-му элементу заставляет генератор вычислить и закешировать значения
    EXPECT_EQ(fib.Get(10), 55);
    EXPECT_EQ(fib.GetMaterializedCount(), 11);

    // Повторный доступ происходит из кэша (O(1))
    EXPECT_EQ(fib.Get(5), 5);

    delete initial;
}
