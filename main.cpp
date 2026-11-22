#include "src/unique_ptr.h"
#include "src/vector.h"
#include <print>

int main() {
    dev::vector<int> vec{{1, 2, 3, 4}};
    for (auto i = 0; i < vec.size(); i++) {
        std::println("Value at index {} is {}", i, vec[i]);
    }

    dev::unique_ptr<int[]> p;
    dev::unique_ptr<int[]> p1{new int[10]};
    std::println("Checking if p is a nullptr: {}",
                 p ? "not nullptr" : "yes nullptr");
    std::println("Checking if p1 is a nullptr: {}",
                 p1 ? "not nullptr" : "yes nullptr");
}
