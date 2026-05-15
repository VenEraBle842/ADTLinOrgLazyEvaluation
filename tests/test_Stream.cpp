#include <gtest/gtest.h>
#include "../src/Stream.h"
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
