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

TEST(LazySequenceTest, FiniteConstructorsAndEmptyGenerator) {
    // 1. Тест конструктора из сырого массива
    int arr[] = {10, 20, 30};
    LazySequence<int> seqFromArray(arr, 3);

    EXPECT_FALSE(seqFromArray.GetCardinality().isInfinite);
    EXPECT_EQ(seqFromArray.GetLength(), 3);
    EXPECT_EQ(seqFromArray.Get(1), 20);
    // Проверяем, что выход за пределы конечной коллекции бросает исключение
    EXPECT_THROW(seqFromArray.Get(3), IndexOutOfRange);

    // 2. Тест полиморфного конструктора (из любой Sequence)
    Sequence<int>* baseSeq = new MutableArraySequence<int>();
    appendTracked(baseSeq, 100);
    appendTracked(baseSeq, 200);

    LazySequence<int> seqFromSeq(baseSeq);
    EXPECT_EQ(seqFromSeq.GetLength(), 2);
    EXPECT_EQ(seqFromSeq.Get(0), 100);
    EXPECT_EQ(seqFromSeq.Get(1), 200);

    // 3. Тест конструктора копирования
    LazySequence<int> seqCopy(seqFromSeq); // NOLINT
    EXPECT_EQ(seqCopy.GetLength(), 2);
    EXPECT_EQ(seqCopy.Get(1), 200);

    // 4. Тест пустого конструктора
    LazySequence<int> emptySeq;
    EXPECT_EQ(emptySeq.GetLength(), 0);
    EXPECT_THROW(emptySeq.Get(0), IndexOutOfRange);

    delete baseSeq;
}
