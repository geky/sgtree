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
        uint8_t left;
        uint8_t right;
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
        new (&_array[0]) node{true, false, false, {K(), V()}};
    }

    ~compact_utree() {
        for (size_t i = _rawsmallest(0); i < _capacity; i = _rawsucc(i)) {
            _array[i].pair.~pair();
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
        return 2*i + 1;
    }

    static size_t _right(size_t i) {
        return 2*i + 2;
    }

    size_t _rawsmallest(size_t i) {
        while (_array[i].left) {
            i = _left(i);
        }
        return i;
    }

    size_t _puresmallest(size_t cap, size_t i) {
        while (_left(i) < cap) {
            i = _left(i);
        }
        return i;
    }

    size_t _smallest(size_t i) {
        i = _rawsmallest(i);
        while (i < _capacity && _array[i].deleted) {
            i = _rawsucc(i);
        }
        return i;
    }

    size_t _rawlargest(size_t i) {
        while (_array[i].right) {
            i = _right(i);
        }
        return i;
    }

    size_t _purelargest(size_t cap, size_t i) {
        while (_right(i) < cap) {
            i = _right(i);
        }
        return i;
    }

    size_t _largest(size_t i) {
        i = _rawlargest(i);
        while (i < _capacity && _array[i].deleted) {
            i = _rawlargest(i);
        }
        return i;
    }

    size_t _rawsucc(size_t i) {
        if (_array[i].right) {
            return _rawsmallest(_right(i));
        } else {
            size_t p = _parent(i);
            while (p < _capacity && i != _left(p)) {
                i = p;
                p = _parent(p);
            }
            return p;
        }
    }

    size_t _puresucc(size_t cap, size_t i) {
        if (_right(i) < cap) {
            return _puresmallest(cap, _right(i));
        } else {
            size_t p = _parent(i);
            while (p < cap && i != _left(p)) {
                i = p;
                p = _parent(p);
            }
            return p;
        }
    }

    size_t _succ(size_t i) {
        i = _rawsucc(i);
        while (i < _capacity && _array[i].deleted) {
            i = _rawsucc(i);
        }
        return i;
    }

    size_t _rawpred(size_t i) {
        if (_array[i].left) {
            return _rawlargest(_left(i));
        } else {
            size_t p = _parent(i);
            while (p < _capacity && i != _right(p)) {
                i = p;
                p = _parent(p);
            }
            return p;
        }
    }

    size_t _purepred(size_t cap, size_t i) {
        if (_left(i) < cap) {
            return _purelargest(cap, _left(i));
        } else {
            size_t p = _parent(i);
            while (p < cap && i != _right(p)) {
                i = p;
                p = _parent(p);
            }
            return p;
        }
    }

    size_t _pred(size_t i) {
        i = _rawpred(i);
        while (i < _capacity && _array[i].deleted) {
            i = _rawpred(i);
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
    void _expand() {
        if (_size > _capacity/2) {
            size_t nheight = _height + 1;
            size_t ncapacity = (1 << _height) - 1;
            node *narray = static_cast<node*>(malloc(ncapacity*sizeof(node)));

            size_t bi = _puresmallest(_size, 0);
            for (size_t i = _rawsmallest(0); i < _capacity; i = _rawsucc(i)) {
                if (_array[i].deleted) {
                    _array[i].pair.~pair();
                    continue;
                }

                new (&narray[bi]) node{
                    false,
                    _left(bi) < _size,
                    _right(bi) < _size,
                    std::move(_array[i].pair)};
                _array[i].pair.~pair();
                bi = _puresucc(_size, bi);
            }

            free(_array);
            _array = narray;
            _height = nheight;
            _capacity = ncapacity;
        } else {
            size_t wi = _purelargest(_capacity, 0);
            for (size_t i = _rawlargest(0); i < _capacity; i = _rawpred(i)) {
                if (_array[i].deleted) {
                    _array[i].pair.~pair();
                    continue;
                }

                if (wi != i) {
                    new (&_array[wi].pair) std::pair<K, V>{
                        std::move(_array[i].pair)};
                    _array[i].pair.~pair();
                }
                wi = _purepred(_capacity, wi);
            }

            size_t bi = _puresmallest(_size, 0);
            wi = _puresucc(_capacity, wi);
            for (size_t i = 0; i < _size; i++) {
                if (bi != wi) {
                    new (&_array[bi]) node{
                        false,
                        _left(bi) < _size,
                        _right(bi) < _size,
                        std::move(_array[wi].pair)};
                    _array[wi].pair.~pair();
                }
                bi = _puresucc(_size, bi);
                wi = _puresucc(_capacity, wi);
            }
        }
    }

public:
    iterator find(const K &k) {
        size_t i = 0;

        while (true) {
            if (_less(k, _array[i].pair.first)) {
                if (!_array[i].left) {
                    return end();
                }
                i = _left(i);
            } else if (_less(_array[i].pair.first, k)) {
                if (!_array[i].right) {
                    return end();
                }
                i = _right(i);
            } else {
                if (_array[i].deleted) {
                    return end();
                }
                return iterator(this, i);
            }
        }
    }

    V &operator[](const K &k) {
        size_t i = 0;
        uint8_t *branch = nullptr;

        while (true) {
            if (_less(k, _array[i].pair.first)) {
                if (!_array[i].left) {
                    branch = &_array[i].left;
                    i = _left(i);
                    break;
                }
                i = _left(i);
            } else if (_less(_array[i].pair.first, k)) {
                if (!_array[i].right) {
                    branch = &_array[i].right;
                    i = _right(i);
                    break;
                }
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

        *branch = true;
        new (&_array[i]) node{false, false, false, {k, V()}};
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
