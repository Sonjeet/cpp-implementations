#include "src/vector.h"
#include <print>

int main() {
    dev::vector<int> vec{{1, 2, 3, 4}};
    for (auto i = 0; i < vec.size(); i++) {
        std::println("Value at index {} is {}", i, vec[i]);
    }
}
