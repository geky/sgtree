
#include "sgtree.hpp"
#include <iostream>

int main() {
    sgtree<int, int> test;
    test.insert(3,3);
    test.insert(1,1);
    test.insert(0,0);
    test.insert(2,2);
    test.insert(5,5);
    test.insert(4,4);
    test.insert(6,6);

    for (int i = 0; i < 28; i++) {
        test[i] = i;
    }


    std::cout << "{ ";
    for (auto &i : test) {
        std::cout << i.val << " ";
    }
    std::cout << "}\n";
}
