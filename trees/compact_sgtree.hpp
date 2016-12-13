/*
 * Compact unbalanced tree stored in an array
 *
 * Copyright (c) 2016 Christopher Haster
 * Distributed under the MIT license
 */

#ifndef COMPACT_SGTREE_HPP
#define COMPACT_SGTREE_HPP

#include <functional>
#include <new>
#include <cstring>
#include <cmath>
#include <cassert>

template <typename K, typename V,
    typename C=std::less<K>,
    typename A=std::ratio<1,2>>
class compact_sgtree;

template <typename K, typename V, typename C=std::less<K>>
using compact_sgtree12 = compact_sgtree<K, V, C, std::ratio<1,2>>;
template <typename K, typename V, typename C=std::less<K>>
using compact_sgtree58 = compact_sgtree<K, V, C, std::ratio<5,8>>;
template <typename K, typename V, typename C=std::less<K>>
using compact_sgtree34 = compact_sgtree<K, V, C, std::ratio<3,4>>;
template <typename K, typename V, typename C=std::less<K>>
using compact_sgtree78 = compact_sgtree<K, V, C, std::ratio<7,8>>;
template <typename K, typename V, typename C=std::less<K>>
using compact_sgtree11 = compact_sgtree<K, V, C, std::ratio<1,1>>;

template <typename K, typename V, typename C, typename A>
class compact_sgtree {
private:
    struct node {
        uint8_t deleted;
        uint8_t left;
        uint8_t right;
        std::pair<K, V> pair;
    };

    C _less;
    constexpr static double _alpha = double(A::num)/double(A::den);

    node *_array;
    size_t _size;
    size_t _height;
    size_t _capacity;

public:
    compact_sgtree()
        : _size(0)
        , _height(3)
        , _capacity((1 << _height) - 1) {
        _array = static_cast<node *>(malloc(_capacity*sizeof(node)));
        new (&_array[0]) node{true, false, false, {K(), V()}};
    }

    ~compact_sgtree() {
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

    static size_t _sibling(size_t i) {
        return ((i+1)^1)-1;
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
    }

    static size_t _bound(size_t root, size_t size) {
        return size + root*(size_t(1) << int(log2(size)));
    }

    void _rebalance(size_t root, size_t w, size_t h) {
        size_t wc = _bound(root, (1 << h) - 1);
        size_t bc = _bound(root, w);

        size_t wi = _purelargest(wc, root);
        size_t ci = _rawlargest(root);
        while (ci+1 > root) {
            if (_array[ci].deleted) {
                _array[ci].pair.~pair();
                ci = _rawpred(ci);
                continue;
            }

            if (wi != ci) {
                new (&_array[wi].pair) std::pair<K, V>{
                    std::move(_array[ci].pair)};
                _array[ci].pair.~pair();
            }

            _array[wi].deleted = 2;
            wi = _purepred(wc, wi);
            ci = _rawpred(ci);
        }

        size_t bi = _puresmallest(bc, root);
        wi = _puresucc(wc, wi);
        while (wi+1 > root) {
            if (bi != wi) {
                new (&_array[bi]) node{
                    false,
                    _left(bi) < bc,
                    _right(bi) < bc,
                    std::move(_array[wi].pair)};
                _array[wi].pair.~pair();
            }

            bi = _puresucc(bc, bi);
            wi = _puresucc(wc, wi);
        }
    }

    size_t _weigh(size_t root) {
        size_t w = 0;
        for (size_t i = _smallest(root); i+1 > root; i = _succ(i)) {
            w += 1;
        }
        return w;
    }

    std::tuple<size_t, size_t, size_t> _scapegoat(size_t i) {
        size_t w = 1;
        size_t h = 1;

        while (true) {
            assert(_parent(i) < _capacity);
            size_t p = _parent(i);
            size_t pw = (_array[p].left && _array[p].right ?
                    _weigh(_sibling(i)) : 0) + w + 1;

            if (w > _alpha * pw + 1) {
                return std::make_tuple(p, pw, h+1);
            }

            i = p;
            w = pw;
            h += 1;
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
        size_t depth = 0;

        while (true) {
            if (_less(k, _array[i].pair.first)) {
                if (!_array[i].left) {
                    branch = &_array[i].left;
                    i = _left(i);
                    break;
                }
                i = _left(i);
                depth += 1;
            } else if (_less(_array[i].pair.first, k)) {
                if (!_array[i].right) {
                    branch = &_array[i].right;
                    i = _right(i);
                    break;
                }
                i = _right(i);
                depth += 1;
            } else {
                if (_array[i].deleted) {
                    _array[i].deleted = false;
                    _array[i].pair = std::pair<K, V>(k, V());
                    _size += 1;
                }
                return _array[i].pair.second;
            }
        }

        if (_size > 0 && depth > (log(_size)/log(1.0/_alpha)) + 2) {
            size_t sg, w, h;
            std::tie(sg, w, h) = _scapegoat(_parent(i));
            _rebalance(sg, w, h);
            return operator[](k);
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

template <typename K, typename V, typename C, typename A>
class compact_sgtree<K, V, C, A>::iterator {
private:
    friend compact_sgtree;
    compact_sgtree *_tree;
    size_t _i;

    iterator(compact_sgtree *tree, size_t i)
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
