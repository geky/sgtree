/*
 * Compact unbalanced tree stored in an array
 *
 * Copyright (c) 2016 Christopher Haster
 * Distributed under the MIT license
 */

#ifndef COMPACT_UTREE_HPP
#define COMPACT_UTREE_HPP

#include <functional>
#include <new>
#include <cstring>

template <typename K, typename V, typename C=std::less<K>>
class compact_utree {
private:
    struct node {
        uint8_t deleted;
        uint8_t exists;
        std::pair<K, V> pair;
    };

    C _less;

    node *_array;
    size_t _size;
    size_t _height;
    size_t _capacity;

public:
    compact_utree()
        : _size(0)
        , _height(3)
        , _capacity((1 << _height) - 1) {
        _array = static_cast<node *>(malloc(_capacity*sizeof(node)));
        memset(_array, 0, _capacity*sizeof(node));
    }

    ~compact_utree() {
        for (size_t i = 0; i < _capacity; i++) {
            if (_array[i].exists) {
                _array[i].pair.~pair();
            }
        }

        free(_array);
    }

    size_t size() const {
        return _size;
    }

private:
    static size_t _parent(size_t i) {
        return (i+1)/2 - 1;
    }

    static size_t _left(size_t i) {
        return 2*(i+1) - 1;
    }

    static size_t _right(size_t i) {
        return 2*(i+1) - 0;
    }

    bool _exists(size_t i) {
        return i < _capacity && _array[i].exists;
    }

    size_t _rawsmallest(size_t i) {
        if (!_exists(i)) {
            return -1;
        } else {
            while (_exists(_left(i))) {
                i = _left(i);
            }
            return i;
        }
    }

    size_t _smallest(size_t i) {
        i = _rawsmallest(i);
        while (_exists(i) && _array[i].deleted) {
            i = _rawsucc(i);
        }
        return i;
    }

    size_t _rawsucc(size_t i) {
        if (!_exists(i)) {
            return -1;
        } else if (_exists(_right(i))) {
            return _rawsmallest(_right(i));
        } else {
            size_t p = _parent(i);
            while (_exists(p) && i != _left(p)) {
                i = p;
                p = _parent(p);
            }
            return p;
        }
    }

    size_t _succ(size_t i) {
        i = _rawsucc(i);
        while (_exists(i) && _array[i].deleted) {
            i = _rawsucc(i);
        }
        return i;
    }

public:
    class iterator;

    iterator begin() {
        return iterator(this, _smallest(0));
    }

    iterator end() {
        return iterator(this, -1);
    }

private:
    void _build(size_t i, std::pair<K, V> *temp, size_t len) {
        if (len == 0) {
            return;
        }

        size_t half = len/2;
        _array[i].exists = true;
        new (&_array[i].pair) std::pair<K, V>(std::move(temp[half]));
        temp[half].~pair();

        _build(_left(i), temp, half);
        _build(_right(i), temp+(half+1), len-(half+1));
    }

    void _expand() {
        std::pair<K, V> *temp = static_cast<std::pair<K, V>*>(
                malloc(_size*sizeof(std::pair<K, V>)));
        size_t j = 0;
        for (size_t i = _smallest(0); i < _capacity; i = _succ(i)) {
            new (&temp[j++]) std::pair<K, V>(std::move(_array[i].pair));
            _array[i].pair.~pair();
        }

        for (size_t i = 0; i < _capacity; i++) {
            if (_array[i].deleted) {
                _array[i].pair.~pair();
            }
        }

        if (_size > _capacity/2) {
            free(_array);

            _height += 1;
            _capacity = (1 << _height) - 1;
            _array = static_cast<node*>(malloc(_capacity*sizeof(node)));
        }

        memset(_array, 0, _capacity*sizeof(node));
        _build(0, temp, _size);
        free(temp);
    }

public:
    iterator find(const K &k) {
        size_t i = 0;

        while (_exists(i)) {
            if (_less(k, _array[i].pair.first)) {
                i = _left(i);
            } else if (_less(_array[i].pair.first, k)) {
                i = _right(i);
            } else {
                if (_array[i].deleted) {
                    return end();
                }
                return iterator(this, i);
            }
        }

        return end();
    }

    V &operator[](const K &k) {
        size_t i = 0;

        while (_exists(i)) {
            if (_less(k, _array[i].pair.first)) {
                i = _left(i);
            } else if (_less(_array[i].pair.first, k)) {
                i = _right(i);
            } else {
                if (_array[i].deleted) {
                    _array[i].deleted = false;
                    _array[i].pair = std::pair<K, V>(k, V());
                    _size += 1;
                }
                return _array[i].pair.second;
            }
        }

        if (i >= _capacity) {
            _expand();
            return operator[](k);
        }

        _array[i].exists = true;
        new (&_array[i].pair) std::pair<K, V>(k, V());
        _size += 1;

        return _array[i].pair.second;
    }

    void erase(iterator p) {
        _array[p._i].deleted = true;
        _size -= 1;
    }
};

template <typename K, typename V, typename C>
class compact_utree<K, V, C>::iterator {
private:
    friend compact_utree;
    compact_utree *_tree;
    size_t _i;

    iterator(compact_utree *tree, size_t i)
        : _tree(tree), _i(i) {
    }

public:
    std::pair<K, V> &operator*() { return _tree->_array[_i].pair; }
    std::pair<K, V> *operator->() { return &_tree->_array[_i].pair; }

    friend bool operator==(const iterator &a, const iterator &b) {
        return a._i == b._i;
    }

    friend bool operator!=(const iterator &a, const iterator &b) {
        return a._i != b._i;
    }

    iterator &operator++() {
        _i = _tree->_succ(_i);
        return *this;
    }

    iterator &operator++(int) {
        iterator old = *this;
        _i = _tree->_succ(_i);
        return old;
    }
};

#endif
