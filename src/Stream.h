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
    size_t position;
    bool isLazy;
    Cardinal length;

public:
    explicit SequenceStream(const Sequence<T>* sequence) : seq(sequence), position(0) {
        auto* lazy = dynamic_cast<const LazySequence<T>*>(seq);
        if (lazy) {
            isLazy = true;
            length = lazy->GetCardinality();
        } else {
            isLazy = false;
            length = Cardinal(seq->GetLength());
        }
    }

    bool IsEndOfStream() const override {
        if (length.isInfinite) return false;
        return position >= length.value;
    }

    T Read() override {
        if (IsEndOfStream()) throw IndexOutOfRange("End of stream");
        return seq->Get(position++);
    }

    size_t GetPosition() const override { return position; }
    bool IsCanSeek() const override { return true; }
    size_t Seek(size_t index) override { return position = index; }
    bool IsCanGoBack() const override { return true; }
    void Open() override { position = 0; }
    void Close() override { position = 0; }
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
