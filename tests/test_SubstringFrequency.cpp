#include <gtest/gtest.h>
#include "../src/SubstringFrequency.h"
#include "ArraySequence.h"
#include "../src/Stream.h"
#include <string>

// Тест 1: Базовое прохождение латинского потока (из старой версии)
TEST(AhoCorasickTest, LatinStreamProcessing) {
    std::string patterns[] = {"he", "she", "his", "hers"};
    AhoCorasick<char> ac(patterns, 4);

    // Подготовка потока в памяти
    Sequence<char>* charSeq = new MutableArraySequence<char>();
    std::string text = "ushers";
    for (char c : text) appendTracked(charSeq, c);

    SequenceInputStream<char> stream(charSeq);
    int* freqs = ac.ProcessStream(&stream);

    // "ushers" содержит: "he" (1), "she" (1), "hers" (1), "his" (0)
    EXPECT_EQ(freqs[0], 1); // he
    EXPECT_EQ(freqs[1], 1); // she
    EXPECT_EQ(freqs[2], 0); // his
    EXPECT_EQ(freqs[3], 1); // hers

    delete[] freqs;
    delete charSeq;
}

// Тест 2: Полноценный поиск Юникод-подстрок (Кириллица / wchar_t)
TEST(AhoCorasickTest, CyrillicUnicodeStreamProcessing) {
    std::wstring patterns[] = {L"привет", L"мир", L"ивет"};
    AhoCorasick<wchar_t> ac(patterns, 3);

    Sequence<wchar_t>* charSeq = new MutableArraySequence<wchar_t>();
    std::wstring text = L"приветмир";
    for (wchar_t c : text) appendTracked(charSeq, c);

    SequenceInputStream<wchar_t> stream(charSeq);
    int* freqs = ac.ProcessStream(&stream);

    EXPECT_EQ(freqs[0], 1); // "привет"
    EXPECT_EQ(freqs[1], 1); // "мир"
    EXPECT_EQ(freqs[2], 1); // "ивет" (суффикс "привет", тоже должно засчитаться!)

    delete[] freqs;
    delete charSeq;
}

// Тест 3: Проверка пересекающихся вхождений (Overlapping)
TEST(AhoCorasickTest, OverlappingOccurrences) {
    std::string patterns[] = {"ana", "nan"};
    AhoCorasick<char> ac(patterns, 2);

    Sequence<char>* charSeq = new MutableArraySequence<char>();
    std::string text = "anana";
    // Текст "anana" содержит:
    // - "ana" дважды (на индексах 0-2 и 2-4)
    // - "nan" один раз (на индексе 1-3)
    for (char c : text) appendTracked(charSeq, c);

    SequenceInputStream<char> stream(charSeq);
    int* freqs = ac.ProcessStream(&stream);

    EXPECT_EQ(freqs[0], 2); // "ana"
    EXPECT_EQ(freqs[1], 1); // "nan"

    delete[] freqs;
    delete charSeq;
}

// Тест 4: Работа с абсолютно пустым потоком
TEST(AhoCorasickTest, EmptyStreamHandling) {
    std::string patterns[] = {"anything"};
    AhoCorasick<char> ac(patterns, 1);

    Sequence<char>* charSeq = new MutableArraySequence<char>(); // Пустой поток
    SequenceInputStream<char> stream(charSeq);
    int* freqs = ac.ProcessStream(&stream);

    EXPECT_EQ(freqs[0], 0);

    delete[] freqs;
    delete charSeq;
}

// Тест 5: Поток без совпадений
TEST(AhoCorasickTest, StreamWithNoMatches) {
    std::string patterns[] = {"abc", "def"};
    AhoCorasick<char> ac(patterns, 2);

    Sequence<char>* charSeq = new MutableArraySequence<char>();
    std::string text = "xyzxyzxyz";
    for (char c : text) appendTracked(charSeq, c);

    SequenceInputStream<char> stream(charSeq);
    int* freqs = ac.ProcessStream(&stream);

    EXPECT_EQ(freqs[0], 0);
    EXPECT_EQ(freqs[1], 0);

    delete[] freqs;
    delete charSeq;
}
