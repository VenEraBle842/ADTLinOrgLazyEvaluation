#include <gtest/gtest.h>
#include "../src/LazySequence.h"
#include "ArraySequence.h"

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
    int arr[] = {10, 20, 30};
    LazySequence<int> seqFromArray(arr, 3);

    EXPECT_FALSE(seqFromArray.GetCardinality().isInfinite);
    EXPECT_EQ(seqFromArray.GetLength(), 3);
    EXPECT_EQ(seqFromArray.Get(1), 20);
    EXPECT_THROW(seqFromArray.Get(3), IndexOutOfRange);

    Sequence<int>* baseSeq = new MutableArraySequence<int>();
    appendTracked(baseSeq, 100);
    appendTracked(baseSeq, 200);

    LazySequence<int> seqFromSeq(baseSeq);
    EXPECT_EQ(seqFromSeq.GetLength(), 2);
    EXPECT_EQ(seqFromSeq.Get(0), 100);
    EXPECT_EQ(seqFromSeq.Get(1), 200);

    LazySequence<int> seqCopy(seqFromSeq); // NOLINT
    EXPECT_EQ(seqCopy.GetLength(), 2);
    EXPECT_EQ(seqCopy.Get(1), 200);

    LazySequence<int> emptySeq;
    EXPECT_EQ(emptySeq.GetLength(), 0);
    EXPECT_THROW(emptySeq.Get(0), IndexOutOfRange);

    delete baseSeq;
}

TEST(LazySequenceTest, LazyMutationsAndImmutability) {
    int arr[] = {1, 2, 3};
    LazySequence<int> base(arr, 3);

    Sequence<int>* appended = base.Append(4);
    EXPECT_EQ(appended->GetLength(), 4);
    EXPECT_EQ(appended->Get(3), 4);

    Sequence<int>* inserted = appended->InsertAt(99, 2);
    EXPECT_EQ(inserted->GetLength(), 5);
    EXPECT_EQ(inserted->Get(2), 99);
    EXPECT_EQ(inserted->Get(3), 3);

    Sequence<int>* removed = inserted->RemoveAt(1);
    EXPECT_EQ(removed->GetLength(), 4);
    EXPECT_EQ(removed->Get(0), 1);
    EXPECT_EQ(removed->Get(1), 99);

    Sequence<int>* remFirst = removed->RemoveFirst();
    EXPECT_EQ(remFirst->GetLength(), 3);
    EXPECT_EQ(remFirst->Get(0), 99);

    Sequence<int>* remLast = remFirst->RemoveLast();
    EXPECT_EQ(remLast->GetLength(), 2);
    EXPECT_EQ(remLast->Get(1), 3);

    // Проверка строгой иммутабельности оригинальной коллекции
    EXPECT_EQ(base.GetLength(), 3);
    EXPECT_EQ(base.Get(0), 1);
    EXPECT_EQ(base.Get(1), 2);
    EXPECT_EQ(base.Get(2), 3);

    delete appended;
    delete inserted;
    delete removed;
    delete remFirst;
    delete remLast;
}

TEST(LazySequenceTest, Subsequence) {
    int arr[] = {10, 20, 30, 40, 50};
    LazySequence<int> base(arr, 5);

    Sequence<int>* sub = base.GetSubsequence(1, 3);
    EXPECT_EQ(sub->GetLength(), 3);
    EXPECT_EQ(sub->Get(0), 20);
    EXPECT_EQ(sub->Get(2), 40);
    EXPECT_THROW(sub->Get(3), IndexOutOfRange);

    delete sub;
}

TEST(LazySequenceTest, InfiniteSequenceConstructorLaziness) {
    Sequence<int>* initial = new MutableArraySequence<int>();
    appendTracked(initial, 0);
    appendTracked(initial, 1);

    LazySequence<int> fib(fibRule, initial, Cardinal::Infinity());

    LazySequence<int> wrapped(&fib);

    EXPECT_TRUE(wrapped.GetCardinality().isInfinite);
    EXPECT_EQ(wrapped.Get(10), 55);

    delete initial;
}

// Проверка итератора и range-based for цикла
TEST(LazySequenceTest, EnumeratorAndRangeBasedFor) {
    int arr[] = {1, 2, 3, 4, 5};
    LazySequence<int> lazy(arr, 5);

    IEnumerator<int>* enumerator = lazy.GetEnumerator();
    EXPECT_TRUE(enumerator->MoveNext());
    EXPECT_EQ(enumerator->Current(), 1);
    EXPECT_TRUE(enumerator->MoveNext());
    EXPECT_EQ(enumerator->Current(), 2);
    delete enumerator;

    int sum = 0;
    for (int item : lazy) {
        sum += item;
    }

    // 1+2+3+4+5 = 15
    EXPECT_EQ(sum, 15);
}
