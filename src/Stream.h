#pragma once
#include "Sequence.h"
#include "LazySequence.h"
#include "Exceptions.h"
#include <fstream>
#include <string>
#include <stdexcept>

// Общий интерфейс потока только для чтения
template <class T>
class ReadOnlyStream {
public:
    virtual ~ReadOnlyStream() = default;
    virtual bool IsEndOfStream() const = 0;
    virtual T Read() = 0;
    virtual size_t GetPosition() const = 0;
    virtual bool IsCanSeek() const = 0;
    virtual size_t Seek(size_t index) = 0;
    virtual bool IsCanGoBack() const = 0;
    virtual void Open() = 0;
    virtual void Close() = 0;
};

// Общий интерфейс потока только для записи
template <class T>
class WriteOnlyStream {
public:
    virtual ~WriteOnlyStream() = default;
    virtual size_t GetPosition() const = 0;
    virtual size_t Write(T item) = 0;
    virtual void Open() = 0;
    virtual void Close() = 0;
};

// Универсальная реализация потока поверх любой коллекции Sequence
template <class T>
class SequenceStream : public ReadOnlyStream<T> {
    const Sequence<T>* seq;
    IEnumerator<T>* enumerator; // Итератор для O(1) последовательного чтения
    size_t position;
    bool isLazy;
    Ordinal length;

public:
    explicit SequenceStream(const Sequence<T>* sequence) : seq(sequence), enumerator(nullptr), position(0) {
        if (auto* lazy = dynamic_cast<const LazySequence<T>*>(seq)) {
            isLazy = true;
            length = lazy->GetOrdinality();
        } else {
            isLazy = false;
            length = Ordinal(seq->GetLength());
        }

        try {
            enumerator = seq->GetEnumerator();
        } catch (...) {
            enumerator = nullptr;
        }
    }

    ~SequenceStream() override {
        delete enumerator;
    }

    bool IsEndOfStream() const override {
        if (length.isInfinite) return false;
        return position >= length.value;
    }

    T Read() override {
        if (IsEndOfStream()) throw IndexOutOfRange("End of stream");

        T val;
        if (enumerator) {
            if (enumerator->MoveNext()) {
                val = enumerator->Current();
            } else {
                throw IndexOutOfRange("End of stream");
            }
        } else {
            val = seq->Get(position); // Fallback
        }

        position++;
        return val;
    }

    size_t GetPosition() const override { return position; }
    bool IsCanSeek() const override { return true; }

    size_t Seek(size_t index) override {
        if (index == position) return position;

        if (enumerator) {
            // Если нужно отмотать назад, сбрасываем итератор в начало
            if (index < position) {
                enumerator->Reset();
                position = 0;
            }
            // Прокручиваем итератор до нужной позиции
            while (position < index) {
                enumerator->MoveNext();
                position++;
            }
        } else {
            position = index;
        }
        return position;
    }

    bool IsCanGoBack() const override { return true; }

    void Open() override {
        if (enumerator) enumerator->Reset();
        position = 0;
    }

    void Close() override {
        if (enumerator) enumerator->Reset();
        position = 0;
    }
};

// Физический файловый поток специально для посимвольного чтения.
// Наследуется от сгенерированного компилятором ReadOnlyStream<char>.
class FileCharStream : public ReadOnlyStream<char> {
    mutable std::ifstream file;
    std::string path;
    size_t position;

public:
    explicit FileCharStream(const std::string& filePath) : path(filePath), position(0) {
        if (!file.is_open()) {
            file.open(path);
        }
    }

    ~FileCharStream() override {
        if (file.is_open()) {
            file.close();
        }
    }

    bool IsEndOfStream() const override {
        return !file.is_open() || file.eof() || file.peek() == EOF;
    }

    char Read() override {
        if (IsEndOfStream()) throw IndexOutOfRange("End of stream");
        char c;
        file.get(c);
        position++;
        return c;
    }

    size_t GetPosition() const override { return position; }
    bool IsCanSeek() const override { return false; }
    size_t Seek(size_t index) override { throw std::logic_error("Cannot seek in FileCharStream"); }
    bool IsCanGoBack() const override { return false; }

    void Open() override {
        if (!file.is_open()) file.open(path);
    }

    void Close() override {
        if (file.is_open()) file.close();
    }
};
