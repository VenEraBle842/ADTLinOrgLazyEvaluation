#include <gtest/gtest.h>
#include "../src/Stream.h"
#include "../src/LazySequence.h"
#include "ArraySequence.h"

TEST(StreamTest, SequenceInputStream) {
    Sequence<int>* arr = new MutableArraySequence<int>();
    appendTracked(arr, 10);
    appendTracked(arr, 20);
    appendTracked(arr, 30);

    SequenceInputStream<int> stream(arr);

    EXPECT_FALSE(stream.IsEndOfStream());
    EXPECT_EQ(stream.Input(), 10);
    EXPECT_EQ(stream.GetPosition(), 1);
    EXPECT_EQ(stream.Input(), 20);

    stream.Seek(0);
    EXPECT_EQ(stream.Input(), 10);

    delete arr;
}

// Тест успешного чтения из ленивой коллекции
TEST(StreamTest, SequenceInputStreamLazy) {
    int arr[] = {100, 200, 300};
    LazySequence<int> lazy(arr, 3);

    SequenceInputStream<int> stream(&lazy);

    EXPECT_FALSE(stream.IsEndOfStream());
    EXPECT_EQ(stream.Input(), 100);
    EXPECT_EQ(stream.GetPosition(), 1);
    EXPECT_EQ(stream.Input(), 200);
    EXPECT_EQ(stream.Input(), 300);
    EXPECT_TRUE(stream.IsEndOfStream());
    EXPECT_THROW(stream.Input(), IndexOutOfRange);
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
        SequenceInputStream<int> stream(&brokenSeq);

        EXPECT_EQ(stream.Input(), 42);
        EXPECT_EQ(stream.Input(), 84);
        EXPECT_TRUE(stream.IsEndOfStream());
    });
}

TEST(StreamTest, SequenceOutputStreamDefaultCreation) {
    SequenceOutputStream<int> outStream;

    EXPECT_EQ(outStream.GetPosition(), 0);

    outStream.Output(10);
    outStream.Output(20);
    outStream.Output(30);

    EXPECT_EQ(outStream.GetPosition(), 3);

    // Забираем итоговую коллекцию для проверки
    Sequence<int>* seq = outStream.GetSequence();
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->GetLength(), 3);
    EXPECT_EQ(seq->Get(0), 10);
    EXPECT_EQ(seq->Get(1), 20);
    EXPECT_EQ(seq->Get(2), 30);

    // Память за seq очистит деструктор SequenceOutputStream,
    // т.к. мы не вызывали ReleaseSequence().
}

TEST(StreamTest, SequenceOutputStreamWrapExisting) {
    // 1. Создаем базовую коллекцию
    Sequence<int>* baseSeq = new MutableArraySequence<int>();

    // Безопасно добавляем элемент (appendTracked сам удалит старый baseSeq, если выделится новая память,
    // и обновит указатель baseSeq)
    appendTracked(baseSeq, 99);

    // 2. Обертываем потоком
    SequenceOutputStream<int> outStream(baseSeq);

    EXPECT_EQ(outStream.GetPosition(), 1);

    outStream.Output(100);
    outStream.Output(101);

    EXPECT_EQ(outStream.GetPosition(), 3);

    // 3. Получаем финальный актуальный указатель
    Sequence<int>* finalSeq = outStream.GetSequence();

    EXPECT_EQ(finalSeq->GetLength(), 3);
    EXPECT_EQ(finalSeq->Get(0), 99);
    EXPECT_EQ(finalSeq->Get(1), 100);
    EXPECT_EQ(finalSeq->Get(2), 101);

    // 4. Очищаем память.
    // Если массив не пересоздавался, то finalSeq == baseSeq, и мы удаляем память baseSeq.
    // Если пересоздавался, старый baseSeq уже удален внутри потока, а мы удаляем новый finalSeq.
    delete finalSeq;
}
