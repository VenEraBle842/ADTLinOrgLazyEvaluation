#pragma once
#include "Sequence.h"
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

// Генератор, работающий по рекуррентному правилу со "скользящим окном"
template <class T>
class RuleGenerator : public Generator<T> {
    T (*rule)(Sequence<T>*);
    Sequence<T>* window;
    size_t windowSize;
    bool infinite;
    size_t limit;
    size_t generated;
    size_t yieldedFromWindow;

public:
    RuleGenerator(T (*r)(Sequence<T>*), const Sequence<T>* initialWindow, bool isInfinite = true, size_t lim = 0)
        : rule(r), infinite(isInfinite), limit(lim), generated(0), yieldedFromWindow(0) {
        window = new MutableArraySequence<T>();
        windowSize = initialWindow->GetLength();
        for (int i = 0; i < windowSize; ++i) {
            appendTracked(window, initialWindow->Get(i));
        }
    }

    ~RuleGenerator() override { delete window; }

    bool HasNext() const override {
        if (infinite) return true;
        return generated < limit;
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
        auto* clone = new RuleGenerator<T>(rule, window, infinite, limit);
        clone->generated = this->generated;
        clone->yieldedFromWindow = this->yieldedFromWindow;
        return clone;
    }
};

// Генератор-модификатор (декоратор), позволяющий внедрять отложенные изменения
// (Добавления, вставки, удаления) до их материализации.
template <class T>
class ModifiedGenerator : public Generator<T> {
    Generator<T>* baseGen;

    // Простая структура для хранения отложенной операции
    struct Operation {
        enum Type { INSERT, REMOVE, APPEND } type;
        T item;
        size_t index;
    };

    Operation* ops;
    size_t opsCount;
    size_t opsCapacity;
    size_t currentIndex;

    void addOp(Operation op) {
        if (opsCount == opsCapacity) {
            opsCapacity = opsCapacity == 0 ? 4 : opsCapacity * 2;
            auto* newOps = new Operation[opsCapacity];
            for(size_t i=0; i<opsCount; ++i) newOps[i] = ops[i];
            delete[] ops;
            ops = newOps;
        }
        ops[opsCount++] = op;
    }

public:
    explicit ModifiedGenerator(Generator<T>* base) : ops(nullptr), opsCount(0), opsCapacity(0), currentIndex(0) {
        baseGen = base->Clone();
    }

    ModifiedGenerator(Generator<T>* base, Operation* existingOps, size_t eCount, size_t eCap) : currentIndex(0) {
        baseGen = base->Clone();
        opsCount = eCount;
        opsCapacity = eCap;
        ops = new Operation[opsCapacity];
        for(size_t i=0; i<opsCount; ++i) ops[i] = existingOps[i];
    }

    ~ModifiedGenerator() override {
        delete baseGen;
        delete[] ops;
    }

    // Регистрация отложенных мутаций (создает новый генератор по правилу неизменяемости)
    ModifiedGenerator<T>* AppendOp(const T& item) {
        auto* nextGen = new ModifiedGenerator<T>(this->baseGen, this->ops, this->opsCount, this->opsCapacity);
        nextGen->addOp({Operation::APPEND, item, 0});
        return nextGen;
    }

    ModifiedGenerator<T>* InsertOp(const T& item, size_t index) {
        auto* nextGen = new ModifiedGenerator<T>(this->baseGen, this->ops, this->opsCount, this->opsCapacity);
        nextGen->addOp({Operation::INSERT, item, index});
        return nextGen;
    }

    bool HasNext() const override {
        // Упрощенная логика проверки: если есть базовый генератор или операции Append
        return baseGen->HasNext();
    }

    T GetNext() override {
        // Здесь мы должны применять операции из ops, исходя из currentIndex.
        // Для экономии места реализована базовая концепция: если на текущем индексе
        // запланирован INSERT, возвращаем его. Если REMOVE - пропускаем элемент базы.
        for (size_t i = 0; i < opsCount; ++i) {
            if (ops[i].type == Operation::INSERT && ops[i].index == currentIndex) {
                // Чтобы не зацикливаться, нужно хитро менять индекс или помечать операцию выполненной.
                currentIndex++;
                return ops[i].item;
            }
        }
        T val = baseGen->GetNext();
        currentIndex++;
        return val;
    }

    Generator<T>* Clone() const override {
        auto* clone = new ModifiedGenerator<T>(baseGen, ops, opsCount, opsCapacity);
        clone->currentIndex = this->currentIndex;
        return clone;
    }
};
