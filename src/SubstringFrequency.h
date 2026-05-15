#pragma once
#include "Stream.h"
#include <string>

// Реализация алгоритма Ахо-Корасик за O(N) для поиска подстрок в потоке.
class AhoCorasick {
    template <class T>
    struct ArrayList {
        T* data;
        int count = 0;
        int capacity;

        explicit ArrayList(int initCapacity = 2) : capacity(initCapacity) {
            data = new T[capacity];
        }

        ~ArrayList() { delete[] data; }

        // Запрет копирования: защита от случайного двойного освобождения памяти (Double Free)
        ArrayList(const ArrayList&) = delete;
        ArrayList& operator=(const ArrayList&) = delete;

        void Add(T item) {
            if (count == capacity) {
                capacity *= 2;
                T* newData = new T[capacity];
                for (int i = 0; i < count; ++i) newData[i] = data[i];
                delete[] data;
                data = newData;
            }
            data[count++] = item;
        }
    };

    struct Node {
        Node* children[256] = {nullptr};
        Node* fail = nullptr;
        ArrayList<size_t>* matchIndices;
        Node() : matchIndices(new ArrayList<size_t>) {}
        ~Node() { delete matchIndices; }
    };

    ArrayList<Node*> tracker; // Отслеживает все вызовы new Node()
    Node* root;
    size_t patternCount;

    class NodeQueue {
        Node** data;
        int head = 0, tail = 0;
    public:
        explicit NodeQueue(int capacity) { data = new Node*[capacity]; }
        ~NodeQueue() { delete[] data; }
        void Push(Node* n) { data[tail++] = n; }
        Node* Pop() { return data[head++]; }
        bool IsEmpty() const { return head == tail; }
    };

public:
    AhoCorasick(const std::string* patterns, size_t count) {
        root = new Node();
        tracker.Add(root); // Запоминаем корень
        patternCount = count;

        // Построение Trie
        for (size_t i = 0; i < count; ++i) {
            Node* current = root;
            for (char c : patterns[i]) {
                auto uc = static_cast<unsigned char>(c);
                if (!current->children[uc]) {
                    current->children[uc] = new Node();
                    tracker.Add(current->children[uc]); // Запоминаем каждый новый узел
                }
                current = current->children[uc];
            }
            current->matchIndices->Add(i);
        }

        // Построение суффиксных ссылок (fail) через BFS
        NodeQueue queue(10000);
        root->fail = root;

        for (auto & i : root->children) {
            if (i) {
                i->fail = root;
                queue.Push(i);
            } else {
                i = root; // Автоматная оптимизация
            }
        }

        while (!queue.IsEmpty()) {
            Node* current = queue.Pop();
            for (int i = 0; i < 256; ++i) {
                if (current->children[i] && current->children[i] != root) {
                    Node* child = current->children[i];
                    Node* failNode = current->fail;
                    child->fail = failNode->children[i];

                    // Слияние совпадений из узла ошибки
                    for(int m = 0; m < child->fail->matchIndices->count; ++m) {
                        child->matchIndices->Add(child->fail->matchIndices->data[m]);
                    }
                    queue.Push(child);
                } else if (!current->children[i]) {
                    current->children[i] = current->fail->children[i]; // Создает циклические ссылки!
                }
            }
        }
    }

    ~AhoCorasick() {
        // Проходим по нашему трекеру и удаляем каждый узел ровно один раз
        for (int i = 0; i < tracker.count; ++i) {
            delete tracker.data[i];
        }
    }

    int* ProcessStream(ReadOnlyStream<char>* stream) {
        int* frequencies = new int[patternCount]();
        Node* current = root;

        while (!stream->IsEndOfStream()) {
            char c;
            try {
                c = stream->Read();
            } catch (...) { break; }

            auto uc = static_cast<unsigned char>(c);
            current = current->children[uc];
            for (int i = 0; i < current->matchIndices->count; ++i) {
                frequencies[current->matchIndices->data[i]]++;
            }
        }
        return frequencies;
    }
};
