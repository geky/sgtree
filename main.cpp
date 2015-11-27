
#include "sgtree.hpp"
#include <iostream>

int main() {
    sgtree<int, int> test;
    test.insert(1,2);
    test.insert(2,3);
    test.insert(0,1);
    test.insert(3,4);
    test.insert(4,5);
    test.insert(5,6);


    std::cout << "{ ";
    for (auto i : test) {
        std::cout << i << " ";
    }
    std::cout << "}\n";
}
