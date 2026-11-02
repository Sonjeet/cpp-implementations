#include "vector.h"
#include <gtest/gtest.h>
#include <string>

TEST(vector_test, construct_and_prefill) {
    dev::vector<int> vec(10, 1);
    EXPECT_EQ(vec.size(), 10);
    EXPECT_EQ(vec.capacity(), 10);
    for (auto i = 0; i < 10; i++) {
        EXPECT_EQ(vec[i], 1);
    }
}

TEST(vector_test, construct_empty_vector_with_reserved_cap) {
    dev::vector<int> vec(10);
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 10);
}

TEST(vector_test, construct_with_init_list) {
    std::initializer_list<int> init = {1, 2, 3, 4, 5};
    dev::vector<int> vec(init);
    EXPECT_EQ(vec.size(), init.size());
    EXPECT_EQ(vec.capacity(), init.size());
    for (auto i = 0; i < 5; i++) {
        EXPECT_EQ(vec[i], init.begin()[i]);
    }
}

TEST(vector_test, copy_constructor) {
    dev::vector<int> other{{1, 2, 3, 4, 5}};
    dev::vector<int> vec{other};

    EXPECT_EQ(vec.size(), other.size());
    EXPECT_EQ(vec.capacity(), other.size());
    for (auto i = 0; i < 5; i++) {
        EXPECT_EQ(vec[i], other[i]);
    }
}

TEST(vector_test, move_constructor) {
    std::initializer_list<int> init{1, 2, 3, 4, 5};
    dev::vector<int> other{};
    auto size = other.size();
    auto cap = other.capacity();
    dev::vector<int> vec{std::move(other)};
    EXPECT_EQ(vec.size(), size);
    EXPECT_EQ(vec.capacity(), cap);
    for (auto i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec[i], init.begin()[i]);
    }
    EXPECT_EQ(other.size(), 0);
    EXPECT_EQ(other.capacity(), 0);
    EXPECT_EQ(other.data(), nullptr);
}

TEST(vector_test, indexing_operations) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
    EXPECT_EQ(vec.at(2), 3);
    EXPECT_EQ(vec.at(3), 4);
    EXPECT_EQ(vec.at(4), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);
    EXPECT_EQ(vec.front(), 1);
    EXPECT_EQ(vec.back(), 5);
}

TEST(vector_test, emplace_back) {
    using Person = std::pair<std::string, int>;
    Person p{"Sonj", 28};
    Person v{"vana", 80};
    dev::vector<Person> vec = {p, p, p, p, p};

    EXPECT_EQ(vec.size(), 5);
    vec.emplace_back("vana", 80);
    EXPECT_EQ(vec.size(), 6);
    for (auto i = 0; i < vec.size() - 1; i++) {
        EXPECT_EQ(vec[i], p);
    }
    EXPECT_EQ(vec[5], vec.back());
    EXPECT_EQ(vec.back(), v);
}

TEST(vector_test, push_back) {
    using Person = std::pair<std::string, int>;
    Person p{"Sonj", 28};
    Person v{"vana", 80};
    dev::vector<Person> vec = {p, p, p, p, p};

    EXPECT_EQ(vec.size(), 5);
    vec.push_back(v);
    EXPECT_EQ(vec.size(), 6);
    for (auto i = 0; i < vec.size() - 1; i++) {
        EXPECT_EQ(vec[i], p);
    }
    EXPECT_EQ(vec[5], vec.back());
    EXPECT_EQ(vec.back(), v);
}

TEST(vector_test, shrink_to_fit) {
    dev::vector<int> vec;
    for (const auto& val : {1, 2, 3, 4, 5}) {
        vec.push_back(val);
    }
    EXPECT_EQ(vec.capacity(), 8);
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 5);
}

TEST(vector_test, resize_to_smaller_count) {
    dev::vector<int> vec{{1, 2, 3, 4, 5, 6}};
    auto old_cap = vec.capacity();
    vec.resize(2);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec.capacity(), old_cap);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
}
TEST(vector_test, resize_to_larger_cap_with_prefill_default_vals) {
    dev::vector<int> vec{{1, 2}};
    vec.resize(5);
    EXPECT_EQ(vec.capacity(), 5);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 0);
    EXPECT_EQ(vec[3], 0);
    EXPECT_EQ(vec[4], 0);
}
TEST(vector_test, resize_to_larger_cap_with_prefill_vals) {
    dev::vector<int> vec{{1, 2}};
    vec.resize(5, 10);
    EXPECT_EQ(vec.capacity(), 5);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 10);
    EXPECT_EQ(vec[3], 10);
    EXPECT_EQ(vec[4], 10);
}
