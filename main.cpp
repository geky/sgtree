
#include "sgtree.hpp"
#include <iostream>
#include <map>
#include <unordered_map>
#include <chrono>
#include <cstdlib>


#define COUNT 1000

#if 0
template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const sgtree<K,V> &tree) {
    os << "{ ";
    for (const auto &i : tree) {
        os << i.val << " ";
    }
    os << "}";

    return os;
}
#else
template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const sgtree<K,V> &tree) {
//    int i = 1;
//    int c = 2;

    std::cout << "[ ";
    for (auto &e : tree.data) {
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
#endif


template <template <typename...> typename T>
void increasing_test() {
    T<unsigned, unsigned> map;
    for (unsigned i = 0; i < COUNT; i++) {
        map[i] = i;
    }

    for (unsigned i = 0; i < COUNT; i++) {
        assert(map[i] == i);
    }
}

template <template <typename...> typename T>
void decreasing_test() {
    T<int, int> map;
    for (int i = 0; i < COUNT; i++) {
        map[-i] = i;
    }

    for (int i = 0; i < COUNT; i++) {
        assert(map[-i] == i);
    }
}

template <template <typename...> typename T>
void random_test() {
    T<int, float> map;
    srand(0);
    for (int i = 0; i < COUNT; i++) {
        float r = rand();
        map[r] = r;
    }

    srand(0);
    for (int i = 0; i < COUNT; i++) {
        float r = rand();
            assert(map[r] == r);
    }
}

template <typename F>
void time(std::string name, F f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << name << ": " << 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count() << "us" << std::endl;
}

int main() {
    time("increasing sgtree", increasing_test<sgtree>);
    time("increasing std::map", increasing_test<std::map>);
    time("increasing std::unordered_map", increasing_test<std::unordered_map>);
    time("decreasing sgtree", decreasing_test<sgtree>);
    time("decreasing std::map", decreasing_test<std::map>);
    time("decreasing std::unordered_map", decreasing_test<std::unordered_map>);
    time("random sgtree", random_test<sgtree>);
    time("random std::map", random_test<std::map>);
    time("random std::unordered_map", random_test<std::unordered_map>);
}

