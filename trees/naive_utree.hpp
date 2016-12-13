/*
 * Simple unbalanced tree
 *
 * Copyright (c) 2016 Christopher Haster
 * Distributed under the MIT license
 */

#ifndef NAIVE_UTREE_HPP
#define NAIVE_UTREE_HPP

#include <functional>

template <typename K, typename V, typename C=std::less<K>>
class naive_utree {
private:
    struct node {
        node *parent;
        node *left;
        node *right;
        std::pair<K, V> pair;
    };

    C _less;

    node *_root;
    size_t _size;

public:
    naive_utree()
        : _root(nullptr)
        , _size(0) {
    }

    ~naive_utree() {
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

        while (n) {
            if (_less(k, n->pair.first)) {
                parent = n;
                branch = &n->left;
                n = n->left;
            } else if (_less(n->pair.first, k)) {
                parent = n;
                branch = &n->right;
                n = n->right;
            } else {
                return n->pair.second;
            }
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

template <typename K, typename V, typename C>
class naive_utree<K, V, C>::iterator {
private:
    friend naive_utree;
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
