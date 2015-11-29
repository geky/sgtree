
#include "sgtree.hpp"
#include <iostream>

template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const sgtree<K,V> &tree) {
    os << "{ ";
    for (const auto &i : tree) {
        os << i.val << " ";
    }
    os << "}";

    return os;
}


int main() {
    sgtree<int, int> test;
    test.insert(3,3);
    test.insert(1,1);
    test.insert(0,0);
    test.insert(2,2);
    test.insert(5,5);
    test.insert(4,4);
    test.insert(6,6);

    for (int i = 0; i < 48; i++) {
        test[i] = i;
    }

    const sgtree<int, int> test2 = test;

    std::cout << test2 << std::endl;
    std::cout << test2[23] << std::endl;
}

