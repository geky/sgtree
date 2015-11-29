/*
 * Compact scapegoat array
 */

#ifndef SGTREE_H
#define SGTREE_H

#include <functional>
#include <algorithm>
#include "maybe.hpp"
#include "buffer.hpp"


#include <iostream>

template <typename K, typename V>
class sgtree;

template <typename K, typename V>
std::ostream &dump(buffer<typename sgtree<K,V>::entry> &b) {
//    int i = 1;
//    int c = 2;

    std::cout << "[ ";
    for (auto &e : b) {
        if (!e) {
            std::cout << "-";
        } else {
            std::cout << e->key;
        }

        std::cout << " ";

//        if (--i == 0) {
//            std::cout << "|";
//            i = c;
//            c *= 2;
//        } else {
//            std::cout << " ";
//        }
    }
    std::cout << "]";

    return std::cout;
}

template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, sgtree<K,V> &tree) {
    os << "{ ";
    for (auto &i : tree) {
        os << i.val << " ";
    }
    os << "}";

    return os;
}


template <typename K, typename V>
class sgtree {
public:
    struct pair { K key; V val; };
    typedef maybe<pair> entry;
    buffer<entry> data;
    unsigned count;

    sgtree(unsigned size = 0) :
            data(size), count(0) {}

private:
    // Normal iteration
    bool valid(unsigned i)       { return i < data.size && data[i]; }
    unsigned left(unsigned i)    { return 2*(i+1)-1; }
    unsigned right(unsigned i)   { return 2*(i+1)-0; }
    unsigned parent(unsigned i)  { return (i+1)/2-1; }
    unsigned sibling(unsigned i) { return ((i+1)^1)-1; }

    // Perfect iteration
    struct perfect {
        unsigned i; unsigned w;
        perfect(unsigned i) : i(i), w(1) {}
        perfect(unsigned i, unsigned w) : i(i), w(2*w) {}
        operator unsigned() { return i; }
    };

    bool valid(perfect i)      { return i.w > 1; }
    perfect left(perfect i)    { i.i = left(i.i); i.w /= 2; return i; }
    perfect right(perfect i)   { i.i = right(i.i); i.w /= 2; return i; }
    perfect parent(perfect i)  { i.i = parent(i.i); i.w *= 2; return i; }
    perfect sibling(perfect i) { i.i = sibling(i.i); return i; }

    // Terrible iteration
    struct terrible {
        unsigned i;
        terrible(unsigned i) : i(i) {}
        operator unsigned() { return i; }
    };

    bool valid(terrible i)       { return i.i < data.size; }
    terrible left(terrible i)    { i.i = left(i.i); return i; }
    terrible right(terrible i)   { i.i = right(i.i); return i; }
    terrible parent(terrible i)  { i.i = parent(i.i); return i; }
    terrible sibling(terrible i) { i.i = sibling(i.i); return i; }

    // Iteration over the actual tree
    template <typename I>
    I smallest(I i) {
        I p = -1;
        while (valid(i)) {
            p = i;
            i = left(p);
        }

        return p;
    }

    template <typename I>
    I largest(I i) {
        I p = -1;
        while (valid(i)) {
            p = i;
            i = right(p);
        }

        return p;
    }

    template <typename I>
    I succ(I i) {
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
    I pred(I i) {
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
    unsigned lookup(const K &key) {
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

    unsigned weight(unsigned i) {
        if (!valid(i)) {
            return 0;
        } else {
            return weight(left(i)) + weight(right(i)) + 1;
        }
    }

    unsigned scapegoat(unsigned i, unsigned *w) {
        unsigned a = *w;
        unsigned b = weight(sibling(i));
        *w = a+b+1;

        if (2*a > *w || 2*b > *w) {
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

    iter begin() {
        return iter(this, smallest(0));
    }

    iter end() {
        return iter(this, -1);
    }

public: // Set operations
    iter find(const K &key) {
        unsigned i = lookup(key);
        if (valid(i)) {
            return iter(this, i);
        } else {
            return iter(this, -1);
        }
    }

    void insert(const K &key, const V &val) {
        unsigned i = lookup(key);
        if (i >= data.size) {
            i = expand(i, key);
        }

        data[i] = entry{pair{key, val}};
        count++;
    }

    void erase(const K &key) {
        unsigned i = lookup(key);
        if (valid(i)) {
            unsigned j = succ(i);
            data[i] = data[j];
            data[j] = entry{};
            count--;
        }
    }

    V &operator[](const K &key) {
        unsigned i = lookup(key);
        if (i >= data.size) {
            i = expand(i, key);
        }

        if (!data[i]) {
            data[i] = entry{pair{key, V{}}};
            count++;
        }

        return data[i]->val;
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


#endif

