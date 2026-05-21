#pragma once
#include "Sequence.h"
#include "LazySequence.h"
#include "Exceptions.h"
#include <fstream>
#include <string>
#include <stdexcept>

// Общий интерфейс потока только для чтения
template <class T>
class InputStream {
public:
    virtual ~InputStream() = default;
    virtual bool IsEndOfStream() const = 0;
    virtual T Input() = 0;
    virtual size_t GetPosition() const = 0;
    virtual bool IsCanSeek() const = 0;
    virtual size_t Seek(size_t index) = 0;
    virtual bool IsCanGoBack() const = 0;
    virtual void Open() = 0;
    virtual void Close() = 0;
};

// Общий интерфейс потока только для записи
template <class T>
class OutputStream {
public:
    virtual ~OutputStream() = default;
    virtual size_t GetPosition() const = 0;
    virtual size_t Output(T item) = 0;
    virtual void Open() = 0;
    virtual void Close() = 0;
};

// Реализация потока чтения поверх любой коллекции Sequence
template <class T>
class SequenceInputStream : public InputStream<T> {
    const Sequence<T>* seq;
    IEnumerator<T>* enumerator; // Итератор для O(1) последовательного чтения
    size_t position;
    bool isLazy;
    Ordinal length;

public:
    explicit SequenceInputStream(const Sequence<T>* sequence) : seq(sequence), enumerator(nullptr), position(0) {
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

    ~SequenceInputStream() override {
        delete enumerator;
    }

    bool IsEndOfStream() const override {
        if (length.isInfinite) return false;
        return position >= length.value;
    }

    T Input() override {
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

// Физический файловый поток специально для посимвольного чтения
// Наследуется от сгенерированного компилятором InputStream<char>
class FileInputStream : public InputStream<char> {
    mutable std::ifstream file;
    std::string path;
    size_t position;

public:
    explicit FileInputStream(const std::string& filePath) : path(filePath), position(0) {
        if (!file.is_open()) file.open(path);
    }

    ~FileInputStream() override {
        if (file.is_open()) file.close();
    }

    bool IsEndOfStream() const override {
        return !file.is_open() || file.eof() || file.peek() == EOF;
    }

    char Input() override {
        if (IsEndOfStream()) throw IndexOutOfRange("End of stream");
        char c;
        file.get(c);
        position++;
        return c;
    }

    size_t GetPosition() const override { return position; }
    bool IsCanSeek() const override { return false; }
    size_t Seek(size_t index) override { throw std::logic_error("Cannot seek in FileInputStream"); }
    bool IsCanGoBack() const override { return false; }

    void Open() override {
        if (!file.is_open()) file.open(path);
    }

    void Close() override {
        if (file.is_open()) file.close();
    }
};

// Реализация потока записи поверх любой коллекции Sequence
// При каждой записи элемент добавляется в конец коллекции
template <class T>
class SequenceOutputStream : public OutputStream<T> {
    Sequence<T>* seq;
    bool ownsSequence; // Флаг владения: нужно ли удалять коллекцию в деструкторе
    size_t position;

public:
    // Если передать существующую коллекцию, поток будет дописывать в нее, иначе создастся MutableArraySequence
    explicit SequenceOutputStream(Sequence<T>* sequence = nullptr) {
        if (sequence) {
            seq = sequence;
            ownsSequence = false;
            position = seq->GetLength();
        } else {
            seq = new MutableArraySequence<T>();
            ownsSequence = true;
            position = 0;
        }
    }

    ~SequenceOutputStream() override {
        if (ownsSequence) delete seq;
    }

    size_t GetPosition() const override { return position; }
    Sequence<T>* GetSequence() const { return seq; }

    // Позволяет забрать коллекцию, чтобы она не удалилась при уничтожении потока
    Sequence<T>* ReleaseSequence() {
        ownsSequence = false;
        return seq;
    }

    size_t Output(T item) override {
        // Функция appendTracked корректно работает как с Mutable, так и с Immutable,
        // обновляя указатель seq и при необходимости удаляя старый объект.
        appendTracked(seq, item);
        position++;
        return position;
    }

    void Open() override {}
    void Close() override {}
};

// Физический файловый поток специально для посимвольной записи
// Наследуется от OutputStream<char>
class FileOutputStream : public OutputStream<char> {
    mutable std::ofstream file;
    std::string path;
    size_t position;

public:
    // По умолчанию может просто перезаписывать (append = false),
    // но можно передать флаг дозаписи в существующий файл.
    explicit FileOutputStream(const std::string& filePath, bool append = false)
        : path(filePath), position(0) {
        auto mode = std::ios::out;
        if (append) mode |= std::ios::app;

        file.open(path, mode);

        if (file.is_open()) {
            auto pos = file.tellp();
            position = pos >= 0 ? static_cast<size_t>(pos) : 0;
        }
    }

    ~FileOutputStream() override {
        if (file.is_open()) file.close();
    }

    size_t GetPosition() const override { return position; }

    size_t Output(char item) override {
        if (!file.is_open()) throw std::logic_error("Cannot output: file stream is closed");
        file.put(item);
        position++;
        return position;
    }

    void Open() override {
        if (!file.is_open()) {
            // Если открываем повторно, по умолчанию делаем append, чтобы не стереть то,
            // что писали до закрытия.
            file.open(path, std::ios::out | std::ios::app);
            auto pos = file.tellp();
            position = pos >= 0 ? static_cast<size_t>(pos) : 0;
        }
    }

    void Close() override {
        if (file.is_open()) file.close();
    }
};
