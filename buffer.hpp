/*
 * Buffer wrapping memory operations
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <new>
#include <cstdlib>
#include <cassert>
#include <iostream>


template <typename T>
struct buffer {
    T *data;
    unsigned size;

    buffer(unsigned size = 0) :
            data((T*)malloc(size*sizeof(T))), size(size) {
        for (unsigned i = 0; i < size; i++) {
            new (&data[i]) T;
        }
    }

    buffer(const buffer &b) :
            data((T*)malloc(b.size*sizeof(T))), size(b.size) {
        for (unsigned i = 0; i < size; i++) {
            new (&data[i]) T{b.data[i]};
        }
    }

    buffer &operator=(const buffer &b) {
        if (this != &b) {
            for (unsigned i = 0; i < size; i++) {
                data[i].~T();
            }
            free(data);

            data = (T*)malloc(b.size*sizeof(T));
            size = b.size;
            for (unsigned i = 0; i < size; i++) {
                new (&data[i]) T{b.data[i]};
            }
        }

        return this;
    }

    ~buffer() {
        for (unsigned i = 0; i < size; i++) {
            data[i].~T();
        }
        free(data);
    }

    void resize(unsigned nsize) {
        for (unsigned i = nsize; i < size; i++) {
            data[i].~T();
        }

        data = (T *)realloc(data, nsize*sizeof(T));
        for (unsigned i = size; i < nsize; i++) {
            new (&data[i]) T;
        }

        size = nsize;
    }

    T &operator[](unsigned i) {
        assert(i < size);
        return data[i]; 
    }

    const T &operator[](unsigned i) const {
        assert(i < size);
        return data[i];
    }

    T *begin()             { return &data[0]; }
    T *end()               { return &data[size]; }
    const T *begin() const { return &data[0]; }
    const T *end() const   { return &data[size]; }
};


#endif
