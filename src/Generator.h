#pragma once
#include "Sequence.h"
#include "Ordinal.h"
#include "Option.h"
#include "Exceptions.h"

// Абстрактный базовый класс Генератора
template <class T>
class Generator {
public:
    virtual ~Generator() = default;

    // Основные методы получения элементов
    virtual T GetNext() = 0;
    virtual bool HasNext() const = 0;
    virtual Generator<T>* Clone() const = 0;

    Option<T> TryGetNext() {
        if (HasNext()) return Option<T>::Some(GetNext());
        return Option<T>::None();
    }
};

// Генератор для конечных последовательностей, заданных статически
template <class T>
class EmptyGenerator : public Generator<T> {
public:
    bool HasNext() const override { return false; }
    T GetNext() override { throw IndexOutOfRange("No elements to generate"); }
    Generator<T>* Clone() const override { return new EmptyGenerator<T>(); }
};

// Генератор, работающий по рекуррентному правилу со "скользящим окном"
template <class T>
class RuleGenerator : public Generator<T> {
    T (*rule)(Sequence<T>*);
    Sequence<T>* window;
    size_t windowSize;
    Ordinal limitOrd;
    size_t generated;
    size_t yieldedFromWindow;

public:
    RuleGenerator(T (*r)(Sequence<T>*), const Sequence<T>* initialWindow, Ordinal ord = Ordinal::Infinity())
        : rule(r), limitOrd(ord), generated(0), yieldedFromWindow(0) {
        window = new MutableArraySequence<T>();
        windowSize = initialWindow->GetLength();
        for (int i = 0; i < windowSize; ++i) {
            appendTracked(window, initialWindow->Get(i));
        }
    }

    ~RuleGenerator() override { delete window; }

    bool HasNext() const override {
        if (limitOrd.isInfinite) return true;
        return generated < limitOrd.value;
    }

    T GetNext() override {
        if (!HasNext()) throw IndexOutOfRange("No more elements in generator");

        // 1. Сначала последовательно отдаем элементы начального окна
        if (yieldedFromWindow < windowSize) {
            T val = window->Get(yieldedFromWindow);
            yieldedFromWindow++;
            generated++;
            return val;
        }

        // 2. Когда окно полностью выдано, начинаем вычислять новые элементы по правилу
        T nextVal = rule(window);
        generated++;

        // Сдвигаем окно
        if (windowSize > 0) {
            Sequence<T>* nextWin = window->RemoveFirst();
            if (nextWin != window) { delete window; window = nextWin; }
            appendTracked(window, nextVal);
        }
        return nextVal;
    }

    Generator<T>* Clone() const override {
        auto* clone = new RuleGenerator<T>(rule, window, limitOrd);
        clone->generated = this->generated;
        clone->yieldedFromWindow = this->yieldedFromWindow;
        return clone;
    }
};

// 1. Адаптер-наблюдатель (View)
// Используется для внешних коллекций (например, когда пользователь передает свой массив).
// Читает элементы через константный указатель.
template <class T>
class SequenceGenerator : public Generator<T> {
    const Sequence<T>* seq;
    size_t index;
    Ordinal lengthOrd;

public:
    SequenceGenerator(const Sequence<T>* s, Ordinal ord, size_t start = 0)
        : seq(s), index(start), lengthOrd(ord) {}

    ~SequenceGenerator() override = default;

    bool HasNext() const override {
        if (lengthOrd.isInfinite) return true;
        return index < lengthOrd.value;
    }

    T GetNext() override {
        if (!HasNext()) throw IndexOutOfRange("No more elements in generator");

        T val = seq->Get(index);
        index++;
        return val;
    }

    Generator<T>* Clone() const override {
        // При клонировании передаем текущий индекс, чтобы клон продолжил с того же места
        return new SequenceGenerator<T>(seq, lengthOrd, index);
    }
};

// 2. Генератор Снимка (Snapshot)
// Создается специально для мутаций.
// Инкапсулирует в себе клон коллекции (снимок) и полностью отвечает за его жизненный цикл.
template <class T>
class SnapshotGenerator : public Generator<T> {
    Sequence<T>* snapshot;
    size_t index;
    Ordinal length;

public:
    SnapshotGenerator(Sequence<T>* seqSnapshot, Ordinal ord, size_t startIdx = 0)
        : snapshot(seqSnapshot), index(startIdx), length(ord) {}

    ~SnapshotGenerator() override {
        delete snapshot; // RAII: удаляем снимок по причине единоличного владения
    }

    bool HasNext() const override {
        if (length.isInfinite) return true;
        return index < length.value;
    }

    T GetNext() override {
        if (!HasNext()) throw IndexOutOfRange("No more elements in SnapshotGenerator");
        return snapshot->Get(index++);
    }

    Generator<T>* Clone() const override {
        // При клонировании создается новый независимый снимок для нового генератора
        return new SnapshotGenerator<T>(snapshot->Clone(), length, index);
    }
};

// Объединяет несколько генераторов. Избегает квадратичной сложности (вложенности деревьев)
// за счет сплющивания (flatten) вложенных ConcatGenerator через метод Merge.
template <class T>
class ConcatGenerator : public Generator<T> {
    Generator<T>** gens;
    size_t count;
    size_t capacity;
    size_t currentIdx;

public:
    ConcatGenerator() : gens(nullptr), count(0), capacity(0), currentIdx(0) {}

    ConcatGenerator(Generator<T>** existing, size_t c, size_t cap, size_t cur)
        : count(c), capacity(cap), currentIdx(cur)
    {
        if (capacity > 0) {
            gens = new Generator<T>*[capacity];
            for (size_t i = 0; i < count; ++i) {
                gens[i] = existing[i]->Clone();
            }
        } else {
            gens = nullptr;
        }
    }

    ~ConcatGenerator() override {
        for (size_t i = 0; i < count; ++i) delete gens[i];
        delete[] gens;
    }

    void AddGenerator(Generator<T>* g) {
        if (count == capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            auto* newGens = new Generator<T>*[capacity];
            for (size_t i = 0; i < count; ++i) newGens[i] = gens[i];
            delete[] gens;
            gens = newGens;
        }
        gens[count++] = g;
    }

    void Merge(const ConcatGenerator<T>* other) {
        for (size_t i = other->currentIdx; i < other->count; ++i) {
            AddGenerator(other->gens[i]->Clone());
        }
    }

    bool HasNext() const override {
        size_t tempIdx = currentIdx;
        while (tempIdx < count) {
            if (gens[tempIdx]->HasNext()) return true;
            tempIdx++;
        }
        return false;
    }

    T GetNext() override {
        while (currentIdx < count) {
            if (gens[currentIdx]->HasNext()) {
                return gens[currentIdx]->GetNext();
            }
            currentIdx++;
        }
        throw IndexOutOfRange("No more elements in ConcatGenerator");
    }

    Generator<T>* Clone() const override {
        return new ConcatGenerator<T>(gens, count, capacity, currentIdx);
    }
};

// Генератор-модификатор (декоратор), позволяющий внедрять отложенные изменения
// (добавления, вставки, удаления) до их материализации.
template <class T>
class ModifiedGenerator : public Generator<T> {
    Generator<T>* baseGen;

    struct Operation {
        enum Type { INSERT, REMOVE, APPEND } type;
        T item;
        size_t index;
        bool executed;
    };

    Operation* ops;
    size_t opsCount;
    size_t opsCapacity;

    mutable size_t outIndex;
    mutable size_t baseIndex;
    mutable size_t appendIndex;

    mutable Option<T> bufferedNext;
    mutable bool hasBufferedItem;

    void addOp(Operation op) {
        if (opsCount == opsCapacity) {
            opsCapacity = opsCapacity == 0 ? 4 : opsCapacity * 2;
            auto* newOps = new Operation[opsCapacity];
            for (size_t i = 0; i < opsCount; ++i) newOps[i] = ops[i];
            delete[] ops;
            ops = newOps;
        }
        ops[opsCount++] = op;
    }

    // Буферизация для работы HasNext с учетом пропусков
    void BufferNext() const {
        if (hasBufferedItem) return;

        while (true) {
            for (size_t i = 0; i < opsCount; ++i) {
                if (ops[i].type == Operation::INSERT && ops[i].index == outIndex && !ops[i].executed) {
                    bufferedNext = Option<T>::Some(ops[i].item);
                    hasBufferedItem = true;
                    ops[i].executed = true;
                    return;
                }
            }

            if (baseGen->HasNext()) {
                T val = baseGen->GetNext();

                bool removed = false;
                for (size_t i = 0; i < opsCount; ++i) {
                    if (ops[i].type == Operation::REMOVE && ops[i].index == baseIndex && !ops[i].executed) {
                        ops[i].executed = true;
                        removed = true;
                        break;
                    }
                }

                baseIndex++;

                if (removed) continue;

                bufferedNext = Option<T>::Some(val);
                hasBufferedItem = true;
                return;
            }

            for (size_t i = appendIndex; i < opsCount; ++i) {
                if (ops[i].type == Operation::APPEND && !ops[i].executed) {
                    bufferedNext = Option<T>::Some(ops[i].item);
                    hasBufferedItem = true;
                    ops[i].executed = true;
                    appendIndex = i + 1;
                    return;
                }
            }

            break;
        }
    }

public:
    explicit ModifiedGenerator(Generator<T>* base)
        : ops(nullptr), opsCount(0), opsCapacity(0),
          outIndex(0), baseIndex(0), appendIndex(0),
          bufferedNext(Option<T>::None()), hasBufferedItem(false)
    {
        baseGen = base->Clone();
    }

    ModifiedGenerator(Generator<T>* base, Operation* existingOps, size_t eCount, size_t eCap)
        : opsCount(eCount), opsCapacity(eCap),
          outIndex(0), baseIndex(0), appendIndex(0),
          bufferedNext(Option<T>::None()), hasBufferedItem(false)
    {
        baseGen = base->Clone();
        if (opsCapacity > 0) {
            ops = new Operation[opsCapacity];
            for (size_t i = 0; i < opsCount; ++i) ops[i] = existingOps[i];
        } else {
            ops = nullptr;
        }
    }

    ~ModifiedGenerator() override {
        delete baseGen;
        delete[] ops;
    }

    ModifiedGenerator<T>* AppendOp(const T& item) {
        auto* nextGen = new ModifiedGenerator<T>(this->baseGen, this->ops, this->opsCount, this->opsCapacity);
        nextGen->addOp({Operation::APPEND, item, 0, false});
        return nextGen;
    }

    ModifiedGenerator<T>* InsertOp(const T& item, size_t index) {
        auto* nextGen = new ModifiedGenerator<T>(this->baseGen, this->ops, this->opsCount, this->opsCapacity);
        nextGen->addOp({Operation::INSERT, item, index, false});
        return nextGen;
    }

    ModifiedGenerator<T>* RemoveOp(size_t index) {
        auto* nextGen = new ModifiedGenerator<T>(this->baseGen, this->ops, this->opsCount, this->opsCapacity);
        nextGen->addOp({Operation::REMOVE, T{}, index, false});
        return nextGen;
    }

    bool HasNext() const override {
        BufferNext();
        return hasBufferedItem;
    }

    T GetNext() override {
        BufferNext();
        if (!hasBufferedItem) {
            throw IndexOutOfRange("No more elements in ModifiedGenerator");
        }
        T val = bufferedNext.GetValue();
        hasBufferedItem = false;
        outIndex++;
        return val;
    }

    Generator<T>* Clone() const override {
        auto* clone = new ModifiedGenerator<T>(baseGen, ops, opsCount, opsCapacity);
        clone->outIndex = this->outIndex;
        clone->baseIndex = this->baseIndex;
        clone->appendIndex = this->appendIndex;
        clone->hasBufferedItem = this->hasBufferedItem;
        if (this->hasBufferedItem) {
            clone->bufferedNext = this->bufferedNext;
        }
        for (size_t i = 0; i < opsCount; ++i) {
            clone->ops[i].executed = this->ops[i].executed;
        }
        return clone;
    }
};
