#include <gtest/gtest.h>
#include "../src/Ordinal.h"

TEST(OrdinalTest, EqualityAndComparisons) {
    Ordinal fin1(5);
    Ordinal fin2(5);
    Ordinal fin3(10);
    Ordinal inf1 = Ordinal::Infinity();
    Ordinal inf2 = Ordinal::Infinity();

    EXPECT_TRUE(fin1 == fin2);
    EXPECT_TRUE(inf1 == inf2);
    EXPECT_FALSE(fin1 == inf1);
    
    EXPECT_TRUE(fin1 < fin3);
    EXPECT_TRUE(fin3 < inf1);
    EXPECT_FALSE(inf1 < inf2);
}

TEST(OrdinalTest, Arithmetic) {
    Ordinal a(10);
    Ordinal b(5);
    Ordinal inf = Ordinal::Infinity();

    EXPECT_EQ((a + b).value, 15);
    EXPECT_TRUE((a + inf).isInfinite);
    EXPECT_EQ((a - b).value, 5);
    EXPECT_TRUE((inf - a).isInfinite);
}

TEST(OrdinalTest, OrdinalAddition) {
    Ordinal fin(5);                        // 5
    Ordinal inf = Ordinal::Infinity();        // 1 * w + 0
    Ordinal inf5(1, 5);              // 1 * w + 5
    Ordinal inf3(1, 3);              // 1 * w + 3

    // 1. Конечное + Конечное = Конечное
    Ordinal sum1 = fin + Ordinal(3);
    EXPECT_FALSE(sum1.isInfinite);
    EXPECT_EQ(sum1.infiniteCount, 0);
    EXPECT_EQ(sum1.value, 8);

    // 2. Конечное + Бесконечное = Бесконечное (конечное слева поглощается)
    // 5 + w = w
    Ordinal sum2 = fin + inf;
    EXPECT_TRUE(sum2.isInfinite);
    EXPECT_EQ(sum2.infiniteCount, 1);
    EXPECT_EQ(sum2.value, 0);

    // 3. Бесконечное + Бесконечное с константами
    // (w + 5) + (w + 3) = 2w + 3
    Ordinal sum3 = inf5 + inf3;
    EXPECT_TRUE(sum3.isInfinite);
    EXPECT_EQ(sum3.infiniteCount, 2);
    EXPECT_EQ(sum3.value, 3);
}

TEST(OrdinalTest, OrdinalComparison) {
    Ordinal fin(1000);
    Ordinal inf(1, 0);
    Ordinal inf5(1, 5);
    Ordinal inf2(2, 0);

    EXPECT_TRUE(fin < inf);       // 1000 < w
    EXPECT_TRUE(inf < inf5);      // w < w + 5
    EXPECT_TRUE(inf5 < inf2);     // w + 5 < 2w
    EXPECT_FALSE(inf2 < inf5);
    EXPECT_TRUE(fin == Ordinal(1000));
}

TEST(OrdinalTest, OrdinalSubtraction) {
    Ordinal fin5(5);
    Ordinal fin3(3);
    Ordinal inf(1, 0);
    Ordinal inf5(1, 5);
    Ordinal inf2(2, 0);

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
