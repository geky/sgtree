/*
 * Compact unbalanced tree stored in a linearized array
 *
 * Copyright (c) 2016 Christopher Haster
 * Distributed under the MIT license
 */

#ifndef LINEAR_UTREE_HPP
#define LINEAR_UTREE_HPP

#include <functional>
#include <new>
#include <cstring>

template <typename K, typename V, typename C=std::less<K>>
class linear_utree {
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
    linear_utree()
        : _size(0)
        , _height(3)
        , _capacity((1 << _height) - 1) {
        _array = static_cast<node *>(malloc(_capacity*sizeof(node)));
        memset(_array, 0, _capacity*sizeof(node));
    }

    ~linear_utree() {
        for (size_t i = 0; i < _capacity; i++) {
            if (_array[i].exists && !_array[i].deleted) {
                _array[i].pair.~pair();
            }
        }

        free(_array);
    }

    size_t size() const {
        return _size;
    }

public:
    class iterator;

    iterator begin() {
        size_t i = 0;
        while (i < _capacity && (!_array[i].exists || _array[i].deleted)) {
            i++;
        }

        return iterator(this, &_array[i]);
    }

    iterator end() {
        return iterator(this, &_array[_capacity]);
    }

private:
    void _build(size_t l, size_t h, std::pair<K, V> *temp, size_t len) {
        if (len == 0) {
            return;
        }

        size_t i = (l + h)/2;
        size_t j = len/2;

        _array[i].exists = true;
        new (&_array[i].pair) std::pair<K, V>(std::move(temp[j]));
        temp[j].~pair();

        _build(l, i, temp, j);
        _build(i+1, h, temp+(j+1), len-(j+1));
    }

    void _expand() {
        std::pair<K, V> *temp = static_cast<std::pair<K, V>*>(
                malloc(_size*sizeof(std::pair<K, V>)));
        size_t j = 0;
        for (size_t i = 0; i < _capacity; i++) {
            if (_array[i].exists) {
                if (!_array[i].deleted) {
                    new (&temp[j++]) std::pair<K, V>(std::move(_array[i].pair));
                }
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
        _build(0, _capacity, temp, _size);
        free(temp);
    }

public:
    iterator find(const K &k) {
        ssize_t l = 0;
        ssize_t h = _capacity;
        size_t i = (l + h) / 2;

        while (l < h && _array[i].exists) {
            if (_less(k, _array[i].pair.first)) {
                h = i;
                i = (l + h) / 2;
            } else if (_less(_array[i].pair.first, k)) {
                l = i+1;
                i = (l + h) / 2;
            } else {
                if (_array[i].deleted) {
                    return end();
                }
                return iterator(this, &_array[i]);
            }
        }

        return end();
    }

    V &operator[](const K &k) {
        ssize_t l = 0;
        ssize_t h = _capacity;
        size_t i = (l + h) / 2;

        while (l < h && _array[i].exists) {
            if (_less(k, _array[i].pair.first)) {
                h = i;
                i = (l + h) / 2;
            } else if (_less(_array[i].pair.first, k)) {
                l = i+1;
                i = (l + h) / 2;
            } else {
                if (_array[i].deleted) {
                    _array[i].deleted = false;
                    _array[i].pair = std::pair<K, V>(k, V());
                    _size += 1;
                }

                return _array[i].pair.second;
            }
        }

        if (l >= h) {
            _expand();
            return operator[](k);
        }

        _array[i].exists = true;
        new (&_array[i].pair) std::pair<K, V>(k, V());
        _size += 1;

        return _array[i].pair.second;
    }

    void erase(iterator p) {
        p._node->deleted = true;
        _size -= 1;
    }
};

template <typename K, typename V, typename C>
class linear_utree<K, V, C>::iterator {
private:
    friend linear_utree;
    node *_node;
    node *_end;

    iterator(linear_utree *tree, node *node)
        : _node(node), _end(&tree->_array[tree->_capacity]) {
    }

public:
    std::pair<K, V> &operator*() { return _node->pair; }
    std::pair<K, V> *operator->() { return &_node->pair; }

    friend bool operator==(const iterator &a, const iterator &b) {
        return a._node == b._node;
    }

    friend bool operator!=(const iterator &a, const iterator &b) {
        return a._node != b._node;
    }

    iterator &operator++() {
        _node += 1;

        while (_node < _end && (!_node->exists || _node->deleted)) {
            _node++;
        }

        return *this;
    }

    iterator &operator++(int) {
        iterator old = *this;
        operator++();
        return old;
    }
};

#endif
