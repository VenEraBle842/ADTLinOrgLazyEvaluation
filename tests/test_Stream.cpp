#include <gtest/gtest.h>
#include "../src/Stream.h"
#include "../src/LazySequence.h"
#include "ArraySequence.h"

TEST(StreamTest, SequenceStreamRead) {
    Sequence<int>* arr = new MutableArraySequence<int>();
    appendTracked(arr, 10);
    appendTracked(arr, 20);
    appendTracked(arr, 30);

    SequenceStream<int> stream(arr);

    EXPECT_FALSE(stream.IsEndOfStream());
    EXPECT_EQ(stream.Read(), 10);
    EXPECT_EQ(stream.GetPosition(), 1);
    EXPECT_EQ(stream.Read(), 20);

    stream.Seek(0);
    EXPECT_EQ(stream.Read(), 10);

    delete arr;
}

// Тест успешного чтения из ленивой коллекции
TEST(StreamTest, SequenceStreamLazyRead) {
    int arr[] = {100, 200, 300};
    LazySequence<int> lazy(arr, 3);

    SequenceStream<int> stream(&lazy);

    EXPECT_FALSE(stream.IsEndOfStream());
    EXPECT_EQ(stream.Read(), 100);
    EXPECT_EQ(stream.GetPosition(), 1);
    EXPECT_EQ(stream.Read(), 200);
    EXPECT_EQ(stream.Read(), 300);
    EXPECT_TRUE(stream.IsEndOfStream());
    EXPECT_THROW(stream.Read(), IndexOutOfRange);
}

// --- Инфраструктура для теста "Defensive Programming" ---
// Создаем "плохую" коллекцию, в которой разработчик забыл реализовать итератор
class BrokenIteratorSequence : public Sequence<int> {
    int data[2] = {42, 84};
public:
    // Имитируем отсутствие реализации
    IEnumerator<int>* GetEnumerator() const override { throw std::logic_error("Not implemented natively"); }

    int GetLength() const override { return 2; }
    const int& Get(int index) const override { return data[index]; }

    // Заглушки чистых виртуальных методов (для компиляции)
    const int& GetFirst() const override { return data[0]; }
    const int& GetLast() const override { return data[1]; }
    Sequence<int>* Append(const int& item) override { return nullptr; }
    Sequence<int>* Prepend(const int& item) override { return nullptr; }
    Sequence<int>* InsertAt(const int& item, int index) override { return nullptr; }
    Sequence<int>* RemoveAt(int index) override { return nullptr; }
    Sequence<int>* RemoveFirst() override { return nullptr; }
    Sequence<int>* RemoveLast() override { return nullptr; }
    Sequence<int>* GetSubsequence(int startIndex, int endIndex) const override { return nullptr; }
    Sequence<int>* Clone() const override { return nullptr; }
    Sequence<int>* Instance() override { return nullptr; }
    Sequence<int>* CreateEmpty() const override { return nullptr; }
};

// Доказ-во, что try-catch спасет программу от краша
TEST(StreamTest, SequenceStreamFallbackDefensiveTest) {
    BrokenIteratorSequence brokenSeq;

    EXPECT_NO_THROW({
        SequenceStream<int> stream(&brokenSeq);

        EXPECT_EQ(stream.Read(), 42);
        EXPECT_EQ(stream.Read(), 84);
        EXPECT_TRUE(stream.IsEndOfStream());
    });
}
