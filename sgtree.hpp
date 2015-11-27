/*
 * Compact scapegoat array
 */

#ifndef SGTREE_H
#define SGTREE_H

#include <functional>
#include "maybe.hpp"
#include "buffer.hpp"


template <typename K, typename V>
class sgtree {
private:
    struct pair { K key; V val; };
    typedef maybe<pair> entry;
    buffer<entry> data;
    unsigned count;

public:
    sgtree(unsigned size = 0) :
            data(size), count(0) {}

private: // Indexing
    bool valid(unsigned i)  {
        return i < data.size && data[i];
    }

    unsigned left(unsigned i)   { return 2*(i+1)-1; }
    unsigned right(unsigned i)  { return 2*(i+1)-0; }
    unsigned parent(unsigned i) { return (i+1)/2-1; }

    unsigned smallest(unsigned i) {
        unsigned p = -1;
        while (valid(i)) {
            p = i;
            i = left(p);
        }

        return p;
    }

    unsigned largest(unsigned i) {
        unsigned p = -1;
        while (valid(i)) {
            p = i;
            i = right(p);
        }

        return p;
    }

    unsigned succ(unsigned i) {
        if (valid(right(i))) {
            return smallest(right(i));
        } else {
            unsigned p = parent(i);
            while (valid(p) && i != left(p)) {
                i = p;
                p = parent(i);
            }

            return p;
        }
    }

    unsigned pred(unsigned i) {
        if (valid(left(i))) {
            return largest(left(i));
        } else {
            unsigned p = parent(i);
            while (valid(p) && i != right(p)) {
                i = p;
                p = parent(i);
            }

            return p;
        }
    }

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
        if (i+1 >= data.size) {
            count = count ? 2*count+1 : 1;
            data.resize(count);
        }

        data[i] = entry{pair{key, val}};
    }

    void erase(const K &key) {
        unsigned i = lookup(key);
        if (i < data.size) {
            data[i] = entry{};
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

    iter(sgtree<K, V> *tree, unsigned i) :
            tree(tree), i(i) {}

public:
    iter(const iter &iter) :
            tree(iter.tree), i(iter.i) {}

    iter &operator=(const iter &iter) {
        if (this != &iter) {
            tree = iter.tree;
            i = iter.i;
        }

        return *this;
    }

    V &operator*() {
        return tree->data[i]->val;
    }

    V *operator->() {
        return &tree->data[i]->val;
    }

    iter &operator++() {
        i = tree->succ(i);
        return *this;
    }

    iter operator++(int) {
        iter p = *this;
        i = tree->succ(i);
        return p;
    }

    iter &operator--() {
        i = tree->pred(i);
        return *this;
    }

    iter operator--(int) {
        iter p = *this;
        i = tree->pred(i);
        return p;
    }

    bool operator==(const iter &iter) {
        return i == iter.i;
    }

    bool operator!=(const iter &iter) {
        return i != iter.i;
    }
};


#endif

