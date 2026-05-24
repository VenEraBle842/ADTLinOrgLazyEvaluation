#pragma once
#include "Stream.h"
#include "Trie.h"
#include <string>

// Шаблонный класс алгоритма Ахо-Корасик за O(N) для поиска подстрок в потоке.
template <class CharT = char>
class AhoCorasick {
    Trie<CharT> trie;
    size_t patternCount;

public:
    // Используем std::basic_string<CharT>, чтобы поддерживать как std::string (char), так и std::wstring (wchar_t)
    AhoCorasick(const std::basic_string<CharT>* patterns, size_t count) : patternCount(count) {
        // 1. Наполняем Бор паттернами
        for (size_t i = 0; i < count; ++i) {
            trie.Insert(patterns[i].c_str(), patterns[i].length(), i);
        }

        // 2. Строим суффиксные ссылки Ахо-Корасик
        trie.BuildFailLinks();
    }

    ~AhoCorasick() = default;

    int* ProcessStream(InputStream<CharT>* stream) {
        int* frequencies = new int[patternCount]();
        typename Trie<CharT>::Node* current = trie.GetRoot();

        while (!stream->IsEndOfStream()) {
            CharT c;
            try {
                c = stream->Input();
            } catch (...) { break; }

            // Переход по символу через NFA-логику дерева
            current = trie.Transition(current, c);

            // Фиксируем совпадения
            int matchCount = current->matchIndices.GetSize();
            for (int i = 0; i < matchCount; ++i) {
                ++frequencies[current->matchIndices.Get(i)];
            }
        }
        return frequencies;
    }
};
