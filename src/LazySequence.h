#pragma once
#include "Sequence.h"
#include "Generator.h"
#include "Ordinal.h"
#include "Exceptions.h"
#include <stdexcept>

// Ленивая коллекция, вычисляющая элементы по мере необходимости и кеширующая их (мемоизация).
// Неизменяема (immutable): мутации возвращают новые LazySequence.
template <class T>
class LazySequence : public Sequence<T> {
    mutable Generator<T>* generator;
    mutable Sequence<T>* memoized;
    Ordinal ordinality;

    // Закрытый конструктор для внутренних операций клонирования
    LazySequence(Generator<T>* gen, const T* mem, size_t c, Ordinal ord) {
        generator = gen;
        memoized = new MutableArraySequence<T>();
        for (size_t i = 0; i < c; ++i) {
            appendTracked(memoized, mem[i]);
        }
        ordinality = ord;
    }

    // Принудительное вычисление до нужного индекса
    void EnsureMaterialized(size_t index) const {
        while (static_cast<size_t>(memoized->GetLength()) <= index && generator->HasNext()) {
            appendTracked(memoized, generator->GetNext());
        }
        if (static_cast<size_t>(memoized->GetLength()) <= index) throw IndexOutOfRange("Index out of bounds");
    }

    // Класс итератора специально для ленивой коллекции
    class LazyEnumerator : public IEnumerator<T> {
        const LazySequence<T>* seq;
        size_t currentIndex;
        bool hasCurrent;
        T currentItem;

    public:
        explicit LazyEnumerator(const LazySequence<T>* sequence)
            : seq(sequence), currentIndex(0), hasCurrent(false) {}

        bool MoveNext() override {
            Ordinal ord = seq->GetOrdinality();
            if (ord.isInfinite || currentIndex < ord.value) {
                currentItem = seq->Get(currentIndex++);
                hasCurrent = true;
                return true;
            }
            hasCurrent = false;
            return false;
        }

        T Current() const override {
            if (!hasCurrent) throw IndexOutOfRange("Enumerator is out of bounds or not started");
            return currentItem;
        }

        void Reset() override {
            currentIndex = 0;
            hasCurrent = false;
        }
    };

public:
    // Конструктор...
    // ... из правила и начального окна
    LazySequence(T (*rule)(Sequence<T>*), const Sequence<T>* initialWindow, Ordinal ord = Ordinal::Infinity()) {
        generator = new RuleGenerator<T>(rule, initialWindow, ord);
        memoized = new MutableArraySequence<T>();
        ordinality = ord;
    }

    // ... пустой
    LazySequence() : LazySequence(new EmptyGenerator<T>(), nullptr, 0, Ordinal(0)) {}

    // ... копирования
    LazySequence(const LazySequence<T>& other) {
        generator = other.generator->Clone();
        memoized = other.memoized->Clone();
        ordinality = other.ordinality;
    }

    // ... из обычного массива
    LazySequence(const T* items, size_t size)
    : LazySequence(new EmptyGenerator<T>(), items, size, Ordinal(size)) {}

    // ... обертки
    explicit LazySequence(const Sequence<T>* seq) {
        memoized = new MutableArraySequence<T>();

        if (auto* lazy = dynamic_cast<const LazySequence<T>*>(seq)) {
            ordinality = lazy->GetOrdinality();
        } else {
            ordinality = Ordinal(seq->GetLength());
        }

        generator = new SequenceGenerator<T>(seq, ordinality, 0);
    }

    ~LazySequence() override {
        delete generator;
        delete memoized;
    }

    Ordinal GetOrdinality() const { return ordinality; }
    size_t GetMaterializedCount() const { return static_cast<size_t>(memoized->GetLength()); }

    // --- Строгая реализация чисто виртуальных методов интерфейса Sequence<T> ---

    const T& GetFirst() const override { return Get(0); }
    const T& GetLast() const override {
        if (ordinality.isInfinite) throw std::logic_error("Cannot get last element of infinite sequence");
        if (ordinality.value == 0) throw IndexOutOfRange("Sequence is empty");
        return Get(static_cast<int>(ordinality.value) - 1);
    }

    const T& Get(int index) const override {
        if (index < 0) throw IndexOutOfRange("Index out of bounds");
        EnsureMaterialized(static_cast<size_t>(index));
        return memoized->Get(index);
    }

    int GetLength() const override {
        if (ordinality.isInfinite) throw std::logic_error("Cannot get length of infinite sequence");
        if (ordinality.value > 0) {
            EnsureMaterialized(ordinality.value - 1);
        }
        return memoized->GetLength();
    }

    // Операции мутации возвращают новую последовательность (Immutable)

    Sequence<T>* Concat(const Sequence<T>* other) override {
        Ordinal otherOrd;
        if (auto* lazyOther = dynamic_cast<const LazySequence<T>*>(other)) {
            otherOrd = lazyOther->GetOrdinality();
        } else {
            otherOrd = Ordinal(other->GetLength());
        }

        Ordinal newOrd = ordinality + otherOrd;
        auto* newGen = new ConcatGenerator<T>();

        if (memoized->GetLength() == 0) {
            if (auto* cgLeft = dynamic_cast<ConcatGenerator<T>*>(this->generator)) {
                newGen->Merge(cgLeft);
            } else {
                newGen->AddGenerator(this->generator->Clone());
            }
        } else {
            newGen->AddGenerator(new SnapshotGenerator<T>(this->Clone(), ordinality));
        }

        if (auto* lazyOther = dynamic_cast<const LazySequence<T>*>(other)) {
            if (lazyOther->GetMaterializedCount() == 0) {
                if (auto* cgRight = dynamic_cast<ConcatGenerator<T>*>(lazyOther->generator)) {
                    newGen->Merge(cgRight);
                } else {
                    newGen->AddGenerator(lazyOther->generator->Clone());
                }
            } else {
                newGen->AddGenerator(new SnapshotGenerator<T>(lazyOther->Clone(), otherOrd));
            }
        } else {
            newGen->AddGenerator(new SequenceGenerator<T>(other, otherOrd, 0));
        }

        return new LazySequence<T>(newGen, nullptr, 0, newOrd);
    }

    Sequence<T>* Append(const T& item) override {
        Generator<T>* baseGen = new SnapshotGenerator<T>(this->Clone(), ordinality);

        auto* modGen = new ModifiedGenerator<T>(baseGen);
        ModifiedGenerator<T>* finalGen = modGen->AppendOp(item);

        delete modGen;
        delete baseGen;

        Ordinal newOrd = ordinality + Ordinal(1);
        return new LazySequence<T>(finalGen, nullptr, 0, newOrd);
    }

    Sequence<T>* Prepend(const T& item) override {
        return InsertAt(item, 0);
    }

    Sequence<T>* InsertAt(const T& item, int index) override {
        if (index < 0) throw IndexOutOfRange("Index out of bounds");
        if (!ordinality.isInfinite && static_cast<size_t>(index) > ordinality.value) {
            throw IndexOutOfRange("Index out of bounds");
        }

        Generator<T>* baseGen = new SnapshotGenerator<T>(this->Clone(), ordinality);

        auto* modGen = new ModifiedGenerator<T>(baseGen);
        ModifiedGenerator<T>* finalGen = modGen->InsertOp(item, static_cast<size_t>(index));

        delete modGen;
        delete baseGen;

        Ordinal newOrd = ordinality + Ordinal(1);
        return new LazySequence<T>(finalGen, nullptr, 0, newOrd);
    }

    Sequence<T>* RemoveFirst() override {
        return RemoveAt(0);
    }

    Sequence<T>* RemoveLast() override {
        if (ordinality.isInfinite) {
            throw std::logic_error("Cannot remove the last element from an infinite sequence");
        }
        if (ordinality.value == 0) {
            throw IndexOutOfRange("Cannot remove from an empty sequence");
        }
        return RemoveAt(static_cast<int>(ordinality.value) - 1);
    }

    Sequence<T>* RemoveAt(int index) override {
        if (index < 0) throw IndexOutOfRange("Index out of bounds");
        if (!ordinality.isInfinite && static_cast<size_t>(index) >= ordinality.value) {
            throw IndexOutOfRange("Index out of bounds");
        }

        Generator<T>* baseGen = new SnapshotGenerator<T>(this->Clone(), ordinality);

        auto* modGen = new ModifiedGenerator<T>(baseGen);
        ModifiedGenerator<T>* finalGen = modGen->RemoveOp(static_cast<size_t>(index));

        delete modGen;
        delete baseGen;

        Ordinal newOrd = ordinality - Ordinal(1);
        return new LazySequence<T>(finalGen, nullptr, 0, newOrd);
    }

    Sequence<T>* GetSubsequence(int startIndex, int endIndex) const override {
        if (startIndex < 0 || endIndex < startIndex) {
            throw IndexOutOfRange("Invalid indices for subsequence");
        }
        if (!ordinality.isInfinite && static_cast<size_t>(endIndex) >= ordinality.value) {
            throw IndexOutOfRange("End index out of bounds");
        }

        EnsureMaterialized(endIndex);
        return memoized->GetSubsequence(startIndex, endIndex);
    }

    Sequence<T>* Clone() const override {
        return new LazySequence<T>(*this);
    }

    Sequence<T>* Instance() override { return new LazySequence<T>(); }
    Sequence<T>* CreateEmpty() const override { return new LazySequence<T>(); }

    IEnumerator<T>* GetEnumerator() const override {
        return new LazyEnumerator(this);
    }
};
