#pragma once
#include "Sequence.h"
#include "Generator.h"
#include "Cardinal.h"
#include "Exceptions.h"
#include <stdexcept>

// Ленивая коллекция, вычисляющая элементы по мере необходимости и кеширующая их (мемоизация).
// Неизменяема (immutable): мутации возвращают новые LazySequence.
template <class T>
class LazySequence : public Sequence<T> {
private:
    mutable Generator<T>* generator;
    mutable T* memoized; // Внутренний массив для максимальной скорости мемоизации
    mutable size_t count;
    mutable size_t capacity;
    Cardinal cardinality;

    // Закрытый конструктор для внутренних операций клонирования
    LazySequence(Generator<T>* gen, T* mem, size_t c, size_t cap, Cardinal card) {
        generator = gen->Clone();
        capacity = cap;
        count = c;
        memoized = new T[capacity];
        for (size_t i = 0; i < count; ++i) memoized[i] = mem[i];
        cardinality = card;
    }

    // Принудительное вычисление до нужного индекса
    void EnsureMaterialized(size_t index) const {
        while (count <= index && generator->HasNext()) {
            if (count == capacity) {
                size_t newCap = capacity == 0 ? 10 : capacity * 2;
                T* newMem = new T[newCap];
                for (size_t i = 0; i < count; ++i) newMem[i] = memoized[i];
                delete[] memoized;
                memoized = newMem;
                capacity = newCap;
            }
            memoized[count++] = generator->GetNext();
        }
        if (count <= index) throw IndexOutOfRange("Index out of bounds");
    }

public:
    // Конструктор из правила и начального окна
    LazySequence(T (*rule)(Sequence<T>*), const Sequence<T>* initialWindow, Cardinal card = Cardinal::Infinity()) {
        generator = new RuleGenerator<T>(rule, initialWindow, card.isInfinite, card.value);
        capacity = 10;
        count = 0;
        memoized = new T[capacity];
        cardinality = card;
    }

    LazySequence() : generator(nullptr), capacity(10), count(0), cardinality(0) {
        memoized = new T[capacity];
    }

    ~LazySequence() override {
        delete generator;
        delete[] memoized;
    }

    Cardinal GetCardinality() const { return cardinality; }
    size_t GetMaterializedCount() const { return count; }

    // --- Строгая реализация чисто виртуальных методов интерфейса Sequence<T> ---

    const T& GetFirst() const override { return Get(0); }
    const T& GetLast() const override { return Get(GetLength() - 1); }

    const T& Get(int index) const override {
        if (index < 0) throw IndexOutOfRange("Index out of bounds");
        EnsureMaterialized(index);
        return memoized[index];
    }

    int GetLength() const override {
        if (cardinality.isInfinite) throw std::logic_error("Cannot get length of infinite sequence");
        EnsureMaterialized(cardinality.value - 1);
        return static_cast<int>(count);
    }

    // Операции мутации возвращают новую последовательность (Immutable)
    Sequence<T>* Append(const T& item) override {
        auto* modGen = new ModifiedGenerator<T>(generator);
        auto* finalGen = modGen->AppendOp(item);
        delete modGen;
        return new LazySequence<T>(finalGen, memoized, count, capacity, cardinality + Cardinal(1));
    }

    Sequence<T>* Prepend(const T& item) override { return InsertAt(item, 0); }

    Sequence<T>* InsertAt(const T& item, int index) override {
        auto* modGen = new ModifiedGenerator<T>(generator);
        auto* finalGen = modGen->InsertOp(item, index);
        delete modGen;
        return new LazySequence<T>(finalGen, memoized, count, capacity, cardinality + Cardinal(1));
    }

    // Эти методы объявлены в Sequence.h, поэтому мы обязаны их переопределить.
    Sequence<T>* RemoveFirst() override { throw std::logic_error("Not implemented natively"); }
    Sequence<T>* RemoveLast() override { throw std::logic_error("Not implemented natively"); }
    Sequence<T>* RemoveAt(int index) override { throw std::logic_error("Not implemented natively"); }
    Sequence<T>* GetSubsequence(int startIndex, int endIndex) const override { throw std::logic_error("Not implemented natively"); }

    Sequence<T>* Clone() const override { return new LazySequence<T>(generator, memoized, count, capacity, cardinality); }
    Sequence<T>* Instance() override { return new LazySequence<T>(); }
    Sequence<T>* CreateEmpty() const override { return new LazySequence<T>(); }
    IEnumerator<T>* GetEnumerator() const override { throw std::logic_error("Not implemented natively"); }
};
