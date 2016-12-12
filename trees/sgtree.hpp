/*
 * Simple scapegoat tree
 *
 * Copyright (c) 2016 Christopher Haster
 * Distributed under the MIT license
 */

#ifndef SGTREE_HPP
#define SGTREE_HPP

#include <functional>
#include <ratio>

template <typename K, typename V,
    typename C=std::less<K>,
    typename A=std::ratio<3,4>>
class sgtree;

template <typename K, typename V, typename C=std::less<K>>
using sgtree12 = sgtree<K, V, C, std::ratio<1,2>>;
template <typename K, typename V, typename C=std::less<K>>
using sgtree58 = sgtree<K, V, C, std::ratio<5,8>>;
template <typename K, typename V, typename C=std::less<K>>
using sgtree34 = sgtree<K, V, C, std::ratio<3,4>>;
template <typename K, typename V, typename C=std::less<K>>
using sgtree78 = sgtree<K, V, C, std::ratio<7,8>>;
template <typename K, typename V, typename C=std::less<K>>
using sgtree11 = sgtree<K, V, C, std::ratio<1,1>>;

template <typename K, typename V, typename C, typename A>
class sgtree {
private:
    struct node {
        node *parent;
        node *left;
        node *right;
        std::pair<K, V> pair;
    };

    C _less;
    constexpr static double _alpha = double(A::num)/double(A::den);

    node *_root;
    size_t _size;

public:
    sgtree()
        : _root(nullptr)
        , _size(0) {
    }

    ~sgtree() {
        _del(_root);
    }

    size_t size() const {
        return _size;
    }

private:
    static node *_smallest(node *n) {
        if (!n) {
            return nullptr;
        } else {
            while (n->left) {
                n = n->left;
            }
            return n;
        }
    }

    static node *_succ(node *n) {
        if (!n) {
            return nullptr;
        } else if (n->right) {
            return _smallest(n->right);
        } else {
            node *p = n->parent;
            while (p && n != p->left) {
                n = p;
                p = p->parent;
            }
            return p;
        }
    }

    static void _del(node *n) {
        if (n) {
            _del(n->left);
            _del(n->right);
            delete n;
        }
    }

    size_t _weigh(node *n) {
        if (!n) {
            return 0;
        }

        return _weigh(n->left) + _weigh(n->right) + 1;
    }

    std::pair<node*, size_t> _scapegoat(node *n) {
        size_t w = 1;

        while (true) {
            node *p = n->parent;
            assert(p);

            node *sibling = (n == p->left) ? p->right : p->left;
            size_t pw = w + _weigh(sibling) + 1;
            if (w > _alpha * pw) {
                return {p, pw};
            }

            n = p;
            w = pw;
        }
    }

    node *_build(node **ns, size_t len, node *p) {
        if (len == 0) {
            return 0;
        }

        size_t i = len/2;
        node *n = ns[i];
        n->parent = p;
        n->left = _build(ns, i, n);
        n->right = _build(ns+(i+1), len-(i+1), n);
        return n;
    }

    node *_rebalance(node *n, size_t w) {
        node **ns = static_cast<node**>(malloc(w*sizeof(node*)));
        node *p = n->parent;
        n = _smallest(n);
        for (size_t i = 0; i < w; i++) {
            ns[i] = n;
            n = _succ(n);
        }

        node *balanced = _build(ns, w, p);
        free(ns);
        return balanced;
    }

public:
    class iterator;

    iterator begin() {
        return iterator(_smallest(_root));
    }

    iterator end() {
        return iterator(nullptr);
    }

public:
    iterator find(const K &k) {
        node *n = _root;

        while (n) {
            if (_less(k, n->pair.first)) {
                n = n->left;
            } else if (_less(n->pair.first, k)) {
                n = n->right;
            } else {
                return iterator(n);
            }
        }

        return end();
    }

    V &operator[](const K &k) {
        node *n = _root;
        node *parent = _root;
        node **branch = &_root;
        size_t depth = 0;

        while (n) {
            if (_less(k, n->pair.first)) {
                parent = n;
                branch = &n->left;
                n = n->left;
                depth += 1;
            } else if (_less(n->pair.first, k)) {
                parent = n;
                branch = &n->right;
                n = n->right;
                depth += 1;
            } else {
                return n->pair.second;
            }
        }

        if (_size > 0 && depth > log(_size)/log(1.0/_alpha)+1) {
            assert(!parent->left && !parent->right);
            std::pair<node*, size_t> sg = _scapegoat(parent);
            node *parent = sg.first->parent;
            node **branch = !parent ? &_root :
                (sg.first == parent->left) ?  &parent->left : &parent->right;
            *branch = _rebalance(sg.first, sg.second);
            return operator[](k);
        }

        n = new node;
        n->parent = parent;
        n->left = nullptr;
        n->right = nullptr;
        n->pair = std::pair<K, V>(k, V());
        *branch = n;
        _size += 1;

        return n->pair.second;
    }

    void erase(iterator p) {
        node *n = p._node;
        if (n->left && n->right) {
            node *r = _smallest(n->right);
            std::swap(r->pair, n->pair);
            n = r;
        }

        node **branch;
        if (!n->parent) {
            branch = &_root;
        } else if (n->parent->left == n) {
            branch = &n->parent->left;
        } else {
            branch = &n->parent->right;
        }

        if (n->left) {
            n->left->parent = n->parent;
            *branch = n->left;
        } else if (n->right) {
            n->right->parent = n->parent;
            *branch = n->right;
        } else {
            *branch = nullptr;
        }

        delete n;
        _size -= 1;
    }
};

template <typename K, typename V, typename C, typename A>
class sgtree<K, V, C, A>::iterator {
private:
    friend sgtree;
    node *_node;

    iterator(node *node)
        : _node(node) {
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
        _node = _succ(_node);
        return *this;
    }

    iterator &operator++(int) {
        iterator old = *this;
        _node = _succ(_node);
        return old;
    }
};

#endif
