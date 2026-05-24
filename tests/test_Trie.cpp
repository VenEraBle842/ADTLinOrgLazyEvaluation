#include <gtest/gtest.h>
#include "../src/Trie.h"
#include <string>

// Тест 1: Базовая вставка в Бор и поиск переходов
TEST(TrieTest, BasicInsertionAndFind) {
    Trie<char> trie;

    // Вставляем слова
    trie.Insert("he", 2, 0);
    trie.Insert("his", 3, 1);

    Trie<char>::Node* root = trie.GetRoot();
    ASSERT_NE(root, nullptr);

    // Проверяем существование первого символа 'h'
    Trie<char>::Node* nodeH = root->FindChild('h');
    ASSERT_NE(nodeH, nullptr);

    // От 'h' должны отходить переходы к 'e' и 'i'
    Trie<char>::Node* nodeE = nodeH->FindChild('e');
    Trie<char>::Node* nodeI = nodeH->FindChild('i');
    ASSERT_NE(nodeE, nullptr);
    ASSERT_NE(nodeI, nullptr);

    // Проверяем несуществующие переходы
    EXPECT_EQ(root->FindChild('x'), nullptr);
    EXPECT_EQ(nodeH->FindChild('x'), nullptr);
}

// Тест 2: Поддержание сортировки детей для бинарного поиска (деления пополам)
TEST(TrieTest, ChildrenAreSortedAlphabetically) {
    Trie<char> trie;

    // Вставляем слова вразброс, чтобы проверить автоматическую сортировку на уровне корня
    trie.Insert("z", 1, 0);
    trie.Insert("a", 1, 1);
    trie.Insert("m", 1, 2);
    trie.Insert("b", 1, 3);

    Trie<char>::Node* root = trie.GetRoot();
    ASSERT_EQ(root->children.GetSize(), 4);

    // Символы обязаны храниться строго по алфавиту для правильной работы бинарного поиска
    EXPECT_EQ(root->children.Get(0).character, 'a');
    EXPECT_EQ(root->children.Get(1).character, 'b');
    EXPECT_EQ(root->children.Get(2).character, 'm');
    EXPECT_EQ(root->children.Get(3).character, 'z');

    // Проверяем, что FindChild успешно находит каждый отсортированный узел
    EXPECT_NE(root->FindChild('a'), nullptr);
    EXPECT_NE(root->FindChild('z'), nullptr);
}

// Тест 3: Расчет суффиксных ссылок
TEST(TrieTest, BuildFailLinks) {
    Trie<char> trie;
    trie.Insert("he", 2, 0);
    trie.Insert("she", 3, 1);
    trie.BuildFailLinks();

    Trie<char>::Node* root = trie.GetRoot();

    // Корень 's' в "she"
    Trie<char>::Node* nodeS = root->FindChild('s');
    ASSERT_NE(nodeS, nullptr);
    // Суффиксная ссылка от 's' (длина 1) ведет в корень
    EXPECT_EQ(nodeS->fail, root);

    // Переход 's' -> 'h' в "she"
    Trie<char>::Node* nodeSH = nodeS->FindChild('h');
    ASSERT_NE(nodeSH, nullptr);

    // Переход 'h' в "he"
    Trie<char>::Node* nodeH = root->FindChild('h');
    ASSERT_NE(nodeH, nullptr);

    // Суффиксная ссылка от префикса "sh" обязана вести в префикс "h"
    EXPECT_EQ(nodeSH->fail, nodeH);
}

// Тест 4: Переход по автомату (Transition) с откатами
TEST(TrieTest, NfaTransitions) {
    Trie<char> trie;
    trie.Insert("he", 2, 0);
    trie.Insert("she", 3, 1);
    trie.BuildFailLinks();

    Trie<char>::Node* root = trie.GetRoot();

    // Начинаем обход: идем по пути "sh"
    Trie<char>::Node* state = root;
    state = trie.Transition(state, 's');
    state = trie.Transition(state, 'h');

    // Мы в состоянии "sh". Если придет неверный символ 'x',
    // автомат должен откатиться по цепочке fail-ссылок обратно в корень
    Trie<char>::Node* nextState = trie.Transition(state, 'x');
    EXPECT_EQ(nextState, root);
}

// Тест 5: Работа Trie с широкими символами (Юникод)
TEST(TrieTest, UnicodeTrieSupport) {
    Trie<wchar_t> trie;
    trie.Insert(L"кириллица", 9, 0);
    trie.BuildFailLinks();

    Trie<wchar_t>::Node* root = trie.GetRoot();
    ASSERT_NE(root, nullptr);

    Trie<wchar_t>::Node* nodeK = root->FindChild(L'к');
    ASSERT_NE(nodeK, nullptr);

    Trie<wchar_t>::Node* nodeI = nodeK->FindChild(L'и');
    ASSERT_NE(nodeI, nullptr);
}
