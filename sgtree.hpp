/*
 * Compact scapegoat array
 */

#ifndef SGTREE_H
#define SGTREE_H

#include "maybe.hpp"
#include "buffer.hpp"
#include <functional>
#include <algorithm>
#include <cassert>


template <typename K, typename V>
class sgtree {
//private:
public:
    struct pair { K key; V val; };
    typedef maybe<pair> entry;
    buffer<entry> data;

public:
    sgtree(unsigned size = 0) : data(size) {}

private:
    // Normal iteration
    bool valid(unsigned i) const       { return i < data.size && data[i]; }
    unsigned left(unsigned i) const    { return 2*(i+1)-1; }
    unsigned right(unsigned i) const   { return 2*(i+1)-0; }
    unsigned parent(unsigned i) const  { return (i+1)/2-1; }
    unsigned sibling(unsigned i) const { return ((i+1)^1)-1; }

    // Perfect iteration
    struct perfect {
        unsigned i, w, d;
        perfect(unsigned i, unsigned w=1, unsigned d=1)
                : i(i), w(w), d(d) {}
        operator unsigned() { return i; }
    };

    bool valid(perfect i) const      { return i.w >= i.d; }
    perfect left(perfect i) const    { return perfect{left(i.i), i.w, 2*i.d}; }
    perfect right(perfect i) const   { return perfect{right(i.i), i.w, 2*i.d+1}; }
    perfect parent(perfect i) const  { return perfect{parent(i.i), i.w, i.d/2}; }

    // Terrible iteration
    struct terrible {
        unsigned i;
        terrible(unsigned i) : i(i) {}
        operator unsigned() { return i; }
    };

    bool valid(terrible i) const       { return i.i < data.size; }
    terrible left(terrible i) const    { return terrible{left(i.i)}; }
    terrible right(terrible i) const   { return terrible{right(i.i)}; }
    terrible parent(terrible i) const  { return terrible{parent(i.i)}; }

    // Iteration over the actual tree
    template <typename I>
    I smallest(I i) const {
        I p = -1;
        while (valid(i)) {
            p = i;
            i = left(p);
        }

        return p;
    }

    template <typename I>
    I largest(I i) const {
        I p = -1;
        while (valid(i)) {
            p = i;
            i = right(p);
        }

        return p;
    }

    template <typename I>
    I succ(I i) const {
        if (valid(right(i))) {
            return smallest(right(i));
        } else {
            I p = parent(i);
            while (p < data.size && i != left(p)) {
                i = p;
                p = parent(i);
            }

            return p;
        }
    }

    template <typename I>
    I pred(I i) const {
        if (valid(left(i))) {
            return largest(left(i));
        } else {
            I p = parent(i);
            while (p < data.size && i != right(p)) {
                i = p;
                p = parent(i);
            }

            return p;
        }
    }

    // General traversal
    unsigned lookup(const K &key) const {
        unsigned i = 0;
        while (valid(i)) {
            if (key < data[i]->key) {
                i = left(i);
            } else if (data[i]->key < key) {
                i = right(i);
            } else {
                return i;
            }
        }

        return i;
    }

    unsigned weight(unsigned i) const {
        if (!valid(i)) {
            return 0;
        } else {
            return weight(left(i)) + weight(right(i)) + 1;
        }
    }

    unsigned scapegoat(unsigned i, unsigned *w) const {
        unsigned a = *w;
        unsigned b = weight(sibling(i));
        *w = a+b+1;

        if (2*a > *w) {
            return parent(i);
        } else {
            return scapegoat(parent(i), w);
        }
    }

    // Rebalancing when scapegoat is present
    unsigned rebalance(unsigned g, unsigned w, const K &key) {
        unsigned n;
        unsigned i = largest(g);
        terrible t = largest(terrible{i});

        for (n = 0; n < w-1; n++) {
            std::swap(data[t], data[i]);
            i = pred(i);
            t = pred(t);
        }

        perfect p = smallest(perfect{g, w});
        t = succ(t);
    
        for (n = 0; n < w-1 && data[t]->key < key; n++) {
            std::swap(data[p], data[t]);
            p = succ(p);
            t = succ(t);
        }

        i = p;
        p = succ(p);

        for (; n < w-1; n++) {
            std::swap(data[p], data[t]);
            p = succ(p);
            t = succ(t);
        }

        return i;
    }

    unsigned expand(unsigned i, const K &key) {
        unsigned w = 1;
        unsigned g = scapegoat(i, &w);

        if (valid(g)) {
            return rebalance(g, w, key);
        } else {
            data.resize(data.size ? 2*data.size+1 : 1);
            return i;
        }
    }

public: // Iterator mechanics
    class iter;
    class const_iter;

    iter begin() { return iter(this, smallest(0)); }
    iter end()   { return iter(this, -1); }
    const_iter begin() const { return const_iter(this, smallest(0)); }
    const_iter end() const   { return const_iter(this, -1); }

public: // Set operations
    iter find(const K &key) {
        unsigned i = lookup(key);
        if (valid(i)) {
            return iter(this, i);
        } else {
            return iter(this, -1);
        }
    }

    const_iter find(const K &key) const {
        unsigned i = lookup(key);
        if (valid(i)) {
            return const_iter(this, i);
        } else {
            return const_iter(this, -1);
        }
    }

    void insert(const K &key, const V &val) {
        unsigned i = lookup(key);
        if (i >= data.size) {
            i = expand(i, key);
        }

        data[i] = entry{pair{key, val}};
    }

    V &operator[](const K &key) {
        unsigned i = lookup(key);
        if (i >= data.size) {
            i = expand(i, key);
        }

        if (!data[i]) {
            data[i] = entry{pair{key, V{}}};
        }

        return data[i]->val;
    }

    const V &operator[](const K &key) const {
        unsigned i = lookup(key);
        assert(valid(i));
        return data[i]->val;
    }

    void erase(const K &key) {
        unsigned i = lookup(key);
        if (valid(i)) {
            unsigned j = succ(i);
            data[i] = data[j];
            data[j] = entry{};
        }
    }
};


// Iterators
template <typename K, typename V>
class sgtree<K, V>::iter {
private:
    friend sgtree<K, V>;
    sgtree<K, V> *tree;
    unsigned i;

    iter(sgtree<K, V> *tree, unsigned i) : tree(tree), i(i) {}

public:
    sgtree<K, V>::pair &operator*() { return *tree->data[i]; }
    sgtree<K, V>::pair *operator->() { return &*tree->data[i]; }

    iter &operator++() { i = tree->succ(i); return *this; }
    iter &operator--() { i = tree->pred(i); return *this; }
    iter operator++(int) { iter p = *this; i = tree->succ(i); return p; }
    iter operator--(int) { iter p = *this; i = tree->pred(i); return p; }

    bool operator==(const iter &iter) { return i == iter.i; }
    bool operator!=(const iter &iter) { return i != iter.i; }
};

template <typename K, typename V>
class sgtree<K, V>::const_iter {
private:
    friend sgtree<K, V>;
    const sgtree<K, V> *tree;
    unsigned i;

    const_iter(const sgtree<K, V> *tree, unsigned i) : tree(tree), i(i) {}

public:
    const sgtree<K, V>::pair &operator*() { return *tree->data[i]; }
    const sgtree<K, V>::pair *operator->() { return &*tree->data[i]; }

    const_iter &operator++() { i = tree->succ(i); return *this; }
    const_iter &operator--() { i = tree->pred(i); return *this; }
    const_iter operator++(int)
        { const_iter p = *this; i = tree->succ(i); return p; }
    const_iter operator--(int)
        { const_iter p = *this; i = tree->pred(i); return p; }

    bool operator==(const const_iter &iter) { return i == iter.i; }
    bool operator!=(const const_iter &iter) { return i != iter.i; }
};


#endif

