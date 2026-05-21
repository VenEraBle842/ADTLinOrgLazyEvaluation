#include <gtest/gtest.h>
#include "../src/SubstringFrequency.h"
#include "ArraySequence.h"
#include "../src/Stream.h"
#include <string>

TEST(AhoCorasickTest, StreamProcessing) {
    std::string patterns[] = {"he", "she", "his", "hers"};
    AhoCorasick ac(patterns, 4);

    // Подготовка потока символов в памяти
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
