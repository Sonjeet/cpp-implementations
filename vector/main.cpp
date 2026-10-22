#include "vector.h"
#include <print>

namespace {
void init_with_prefill() {
    dev::vector<int> vec(10, 42);
    auto data = vec.data();
    for (int i = 0; i < vec.size(); i++) {
        std::print("val is: {0}\n", vec.at(i));
    }
}

dev::vector<int> init_with_list() {
    dev::vector<int> vec{{10, 20, 30, 40, 50}};
    return vec;
}

template <typename T> void print_vector(dev::vector<T> vec) {
    for (auto i = 0; i < vec.size(); i++) {
        std::print("Item {0} in vec = {1}", i, vec.at(i));
    }
}
} // namespace

int main() {
    std::print("starting app");
    // auto vec = init_with_list();
    dev::vector<int> vec{{10, 20, 30, 40, 50}};
    for (auto i = 0; i < vec.size(); i++) {
        std::print("Item {0} in vec = {1}\n", i, vec.at(i));
    }
}
