#pragma once
#include "DynamicArray.h"
#include <stdexcept>

// Шаблонный класс Префиксного Дерева (Бора) с поддержкой разреженных переходов
template <class CharT>
class Trie {
public:
    // Узел дерева
    struct Node {
        // Пара "Символ -> Узел" для разреженного хранения переходов
        struct Child {
            CharT character;
            Node* node;

            bool operator<(const Child& other) const { return character < other.character; }
            bool operator==(const Child& other) const { return character == other.character; }
        };

        DynamicArray<Child> children;      // Список только реальных переходов
        Node* fail;                        // Суффиксная ссылка
        DynamicArray<size_t> matchIndices; // Индексы совпавших паттернов

        Node() : children(0), fail(nullptr), matchIndices(0) {}

        // Бинарный поиск перехода по символу (деление пополам) за O(log K)
        Node* FindChild(CharT c) const {
            int low = 0;
            int high = children.GetSize() - 1;
            while (low <= high) {
                int mid = low + (high - low) / 2;
                CharT midChar = children.Get(mid).character;
                if (midChar == c) {
                    return children.Get(mid).node;
                }
                if (midChar < c) {
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }
            return nullptr;
        }

        // Вставка нового дочернего элемента с сохранением сортировки
        void AddChild(CharT c, Node* childNode) {
            int low = 0;
            int high = children.GetSize() - 1;
            int insertIdx = 0;
            while (low <= high) {
                int mid = low + (high - low) / 2;
                CharT midChar = children.Get(mid).character;
                if (midChar == c) {
                    children.Set(mid, {c, childNode});
                    return;
                }
                if (midChar < c) {
                    low = mid + 1;
                    insertIdx = low;
                } else {
                    high = mid - 1;
                    insertIdx = mid;
                }
            }

            // Раздвигаем массив и вставляем на нужное место для сохранения сортировки
            int size = children.GetSize();
            children.Resize(size + 1);
            for (int i = size; i > insertIdx; --i) {
                children.Set(i, children.Get(i - 1));
            }
            children.Set(insertIdx, {c, childNode});
        }
    };

private:
    Node* root;
    DynamicArray<Node*> tracker; // Отслеживает все аллокации для деструктора

    // Вспомогательная очередь для обхода в ширину (BFS)
    class NodeQueue {
        DynamicArray<Node*> data;
        int head = 0, tail = 0;
    public:
        explicit NodeQueue(int capacity) : data(capacity) {}
        void Push(Node* n) {
            if (tail >= data.GetSize()) {
                data.Resize(tail + 1);
            }
            data.Set(tail++, n);
        }
        Node* Pop() { return data.Get(head++); }
        bool IsEmpty() const { return head == tail; }
    };

public:
    Trie() : tracker(0) {
        root = new Node();
        tracker.Resize(1);
        tracker.Set(0, root);
    }

    ~Trie() {
        int size = tracker.GetSize();
        for (int i = 0; i < size; ++i) {
            delete tracker.Get(i);
        }
    }

    Node* GetRoot() const { return root; }

    // Вставка строки (шаблона) в Бор
    void Insert(const CharT* pattern, size_t length, size_t patternIdx) {
        Node* current = root;
        for (size_t i = 0; i < length; ++i) {
            CharT c = pattern[i];
            Node* child = current->FindChild(c);
            if (!child) {
                child = new Node();
                int trackerSize = tracker.GetSize();
                tracker.Resize(trackerSize + 1);
                tracker.Set(trackerSize, child);

                current->AddChild(c, child);
            }
            current = child;
        }
        int matchSize = current->matchIndices.GetSize();
        current->matchIndices.Resize(matchSize + 1);
        current->matchIndices.Set(matchSize, patternIdx);
    }

    // Построение суффиксных ссылок по классическому алгоритму Ахо-Корасик (BFS)
    void BuildFailLinks() {
        NodeQueue queue(tracker.GetSize());
        root->fail = root;

        int rootChildrenSize = root->children.GetSize();
        for (int i = 0; i < rootChildrenSize; ++i) {
            Node* child = root->children.Get(i).node;
            child->fail = root;
            queue.Push(child);
        }

        while (!queue.IsEmpty()) {
            Node* current = queue.Pop();
            int currentChildrenSize = current->children.GetSize();

            for (int i = 0; i < currentChildrenSize; ++i) {
                auto edge = current->children.Get(i); // Безопасно выводим тип через auto
                Node* child = edge.node;
                CharT c = edge.character;

                Node* failNode = current->fail;
                while (failNode != root && !failNode->FindChild(c)) {
                    failNode = failNode->fail;
                }

                Node* candidate = failNode->FindChild(c);
                if (candidate && candidate != child) {
                    child->fail = candidate;
                } else {
                    child->fail = root;
                }

                int failMatchCount = child->fail->matchIndices.GetSize();
                for (int m = 0; m < failMatchCount; ++m) {
                    size_t matchIdx = child->fail->matchIndices.Get(m);
                    int childMatchSize = child->matchIndices.GetSize();
                    child->matchIndices.Resize(childMatchSize + 1);
                    child->matchIndices.Set(childMatchSize, matchIdx);
                }

                queue.Push(child);
            }
        }
    }

    Node* Transition(Node* current, CharT c) const {
        while (current != nullptr) {
            Node* next = current->FindChild(c);
            if (next != nullptr) {
                return next;
            }
            if (current == root) {
                return root;
            }
            current = current->fail;
        }
        return root;
    }
};
