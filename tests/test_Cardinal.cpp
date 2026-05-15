#include <gtest/gtest.h>
#include "../src/Cardinal.h"

TEST(CardinalTest, EqualityAndComparisons) {
    Cardinal fin1(5);
    Cardinal fin2(5);
    Cardinal fin3(10);
    Cardinal inf1 = Cardinal::Infinity();
    Cardinal inf2 = Cardinal::Infinity();

    EXPECT_TRUE(fin1 == fin2);
    EXPECT_TRUE(inf1 == inf2);
    EXPECT_FALSE(fin1 == inf1);
    
    EXPECT_TRUE(fin1 < fin3);
    EXPECT_TRUE(fin3 < inf1);
    EXPECT_FALSE(inf1 < inf2);
}

TEST(CardinalTest, Arithmetic) {
    Cardinal a(10);
    Cardinal b(5);
    Cardinal inf = Cardinal::Infinity();

    EXPECT_EQ((a + b).value, 15);
    EXPECT_TRUE((a + inf).isInfinite);
    EXPECT_EQ((a - b).value, 5);
    EXPECT_TRUE((inf - a).isInfinite);
}
