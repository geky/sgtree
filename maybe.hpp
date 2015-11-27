/*
 * Maybe class with optimization of using null pointers
 */

#ifndef MAYBE_H
#define MAYBE_H

#include <cassert>


template <typename T>
struct maybe {
private:
    bool nil;
    union { T val; };

public:
    maybe() : nil(true) {}

    maybe(const T &t) : nil(false) {
        new (&val) T{t};
    }

    maybe(const maybe &m) : nil(m.nil) {
        if (!nil) {
            new (&val) T{m._val};
        }
    }

    ~maybe() {
        if (!nil) {
            val.~T();
        }
    }

    maybe &operator=(const T &t) {
        if (!nil) {
            val.~T();
        } else {
            nil = false;
        }

        new (&val) T{t};
        return *this;
    }

    maybe &operator=(const maybe &m) {
        if (this != &m) {
            if (!nil) {
                val.~T();
            }

            nil = m.nil;
            if (!nil) {
                new (&val) T{*m};
            }
        }
        return *this;
    }

    operator bool() const {
        return !nil;
    }

    T &operator*() {
        assert(!nil);
        return val;
    }

    const T &operator*() const {
        assert(!nil);
        return val;
    }

    T *operator->() {
        assert(!nil);
        return &val;
    }

    const T *operator->() const {
        assert(!nil);
        return &val;
    }
};


template <typename T>
struct maybe<T *> {
private:
    T *ptr;

public:
    maybe() : ptr(0) {}
    maybe(const T *&t) : ptr(t) {}
    maybe(const maybe &m) : ptr(m._ptr) {}

    maybe &operator=(const T *&t) {
        ptr = t;
        return *this;
    }

    maybe &operator=(const maybe &m) {
        ptr = m._ptr;
        return *this;
    }

    operator bool() const {
        return ptr;
    }

    T *&operator*() {
        assert(ptr);
        return ptr;
    }

    T **operator->() {
        assert(ptr);
        return &ptr;
    }
};


#endif

