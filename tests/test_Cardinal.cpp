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

TEST(CardinalTest, OrdinalAddition) {
    Cardinal fin(5);                        // 5
    Cardinal inf = Cardinal::Infinity();        // 1 * w + 0
    Cardinal inf5(1, 5);              // 1 * w + 5
    Cardinal inf3(1, 3);              // 1 * w + 3

    // 1. Конечное + Конечное = Конечное
    Cardinal sum1 = fin + Cardinal(3);
    EXPECT_FALSE(sum1.isInfinite);
    EXPECT_EQ(sum1.infiniteCount, 0);
    EXPECT_EQ(sum1.value, 8);

    // 2. Конечное + Бесконечное = Бесконечное (конечное слева поглощается)
    // 5 + w = w
    Cardinal sum2 = fin + inf;
    EXPECT_TRUE(sum2.isInfinite);
    EXPECT_EQ(sum2.infiniteCount, 1);
    EXPECT_EQ(sum2.value, 0);

    // 3. Бесконечное + Бесконечное с константами
    // (w + 5) + (w + 3) = 2w + 3
    Cardinal sum3 = inf5 + inf3;
    EXPECT_TRUE(sum3.isInfinite);
    EXPECT_EQ(sum3.infiniteCount, 2);
    EXPECT_EQ(sum3.value, 3);
}

TEST(CardinalTest, OrdinalComparison) {
    Cardinal fin(1000);
    Cardinal inf(1, 0);
    Cardinal inf5(1, 5);
    Cardinal inf2(2, 0);

    EXPECT_TRUE(fin < inf);       // 1000 < w
    EXPECT_TRUE(inf < inf5);      // w < w + 5
    EXPECT_TRUE(inf5 < inf2);     // w + 5 < 2w
    EXPECT_FALSE(inf2 < inf5);
    EXPECT_TRUE(fin == Cardinal(1000));
}

TEST(CardinalTest, OrdinalSubtraction) {
    Cardinal fin5(5);
    Cardinal fin3(3);
    Cardinal inf(1, 0);
    Cardinal inf5(1, 5);
    Cardinal inf2(2, 0);

    // 5 - 3 = 2
    EXPECT_EQ((fin5 - fin3).value, 2);

    // w - 5 = w (по правилу левого вычитания: 5 + w = w)
    EXPECT_EQ((inf - fin5).infiniteCount, 1);
    EXPECT_EQ((inf - fin5).value, 0);

    // 2w - w = w
    EXPECT_EQ((inf2 - inf).infiniteCount, 1);

    // Меньшее минус большее = 0
    EXPECT_EQ((fin3 - fin5).value, 0);
    EXPECT_EQ((inf - inf2).infiniteCount, 0);
}
