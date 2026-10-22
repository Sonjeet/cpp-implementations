#include "vector.h"
#include <gtest/gtest.h>

TEST(vector_test, construct_with_init_list) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);
}

TEST(vector_test, construct_with_prefill) {
    dev::vector<int> vec(5, 1);
    EXPECT_EQ(vec.size(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(vec[i], 1);
    }
}

TEST(vector_test, construct_copy) {
    dev::vector<int> other{{1, 2, 3, 4, 5}};
    dev::vector<int> vec(other);

    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);

    EXPECT_EQ(vec.size(), other.size());
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(vec[i], other[i]);
    }
}

TEST(vector_test, construct_move) {
    dev::vector<int> other{{1, 2, 3, 4, 5}};
    dev::vector<int> vec(std::move(other));

    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);

    EXPECT_EQ(other.size(), 0);
    EXPECT_EQ(other.capacity(), 0);
    EXPECT_EQ(other.data(), nullptr);
}

TEST(vector_test, copy_assignment) {
    dev::vector<int> other{{1, 2, 3, 4, 5}};
    dev::vector<int> vec = other;
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);

    EXPECT_EQ(vec.size(), other.size());
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(vec[i], other[i]);
    }
}

TEST(vector_test, copy_assignment_with_smaller_vector) {
    dev::vector<int> other{{1, 2}};
    dev::vector<int> vec{{10, 20, 30, 40, 50}};

    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 10);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 30);
    EXPECT_EQ(vec[3], 40);
    EXPECT_EQ(vec[4], 50);

    vec = other;
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec.capacity(), 5);
}

TEST(vector_test, access_via_at) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
    EXPECT_EQ(vec.at(2), 3);
    EXPECT_EQ(vec.at(3), 4);
    EXPECT_EQ(vec.at(4), 5);

    vec.at(2) = 30;
    EXPECT_EQ(vec.at(2), 30);
}

TEST(vector_test, push_back) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    vec.push_back(10);

    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[5], 10);
}

TEST(vector_test, push_front) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    vec.push_front(10);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec[0], 10);
}

TEST(vector_test, shrink_to_fit) {
    dev::vector<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    vec.push_back(40);
    vec.push_back(50);
    vec.push_back(60);
    EXPECT_EQ(vec.capacity(), 8);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 6);
}

TEST(vector_test, pop_back) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    vec.pop_back();

    EXPECT_EQ(vec.size(), 4);
    EXPECT_EQ(vec.capacity(), 5);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
}

TEST(vector_test, reserve) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    vec.reserve(100);

    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec.capacity(), 100);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);
}

TEST(vector_test, emplace_back) {
    using Pair = std::pair<int, bool>;
    dev::vector<Pair> vec;
    vec.emplace_back(1, true);
    vec.emplace_back(2, true);
    vec.emplace_back(3, true);
    vec.emplace_back(4, true);
    vec.emplace_back(5, true);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0].first, 1);
    EXPECT_EQ(vec[1].first, 2);
    EXPECT_EQ(vec[2].first, 3);
    EXPECT_EQ(vec[3].first, 4);
    EXPECT_EQ(vec[4].first, 5);
    EXPECT_EQ(vec[0].second, true);
    EXPECT_EQ(vec[1].second, true);
    EXPECT_EQ(vec[2].second, true);
    EXPECT_EQ(vec[3].second, true);
    EXPECT_EQ(vec[4].second, true);
}

TEST(vector_test, insert_at_pos) {
    dev::vector<int> vec{{1, 2, 3, 4, 5}};
    auto it = vec.begin();
    it++;
    vec.insert(it, 10);

    EXPECT_EQ(vec.size(), 6);
}

/*
  * ---- Construction / destruction / assignment ----
  * - vector()                                   default
  * - vector(const Allocator&)                   with allocator
  * - vector(size_type count)                    count value-initialised
 elements
  * - vector(size_type count, const T& value)    count copies of value
  * - vector(InputIt first, InputIt last)        from iterator range
  * - vector(std::from_range_t, R&& range)       from range (C++23)
  * - vector(const vector&)                      copy
  * - vector(vector&&)                           move
  * - vector(const vector&, const Allocator&)    allocator-extended copy
  * - vector(vector&&, const Allocator&)         allocator-extended move
  * - vector(std::initializer_list<T>)           from initializer list
  * - ~vector()                                  destructor
  * - operator=(const vector&)                   copy assignment
  * - operator=(vector&&)                        move assignment
  * - operator=(std::initializer_list<T>)        initializer-list assignment
  * - assign(size_type count, const T& value)    replace contents with count
 copies
  * - assign(InputIt first, InputIt last)        replace contents from range
  * - assign(std::initializer_list<T>)           replace contents from list
  * - assign_range(R&& range)                    replace contents from range
 (C++23)
  * - get_allocator()                            copy of stored allocator
  *
  * ---- Element access ----
  * - at(size_type)                              bounds-checked, throws
 std::out_of_range
  * - operator[](size_type)                      unchecked
  * - front()                                    first element (UB if empty)
  * - back()                                     last element (UB if empty)
  * - data()                                     pointer to underlying array
  *
  * ---- Iterators ----
  * - begin() / cbegin()
  * - end() / cend()
  * - rbegin() / crbegin()
  * - rend() / crend()
  *
  * ---- Capacity ----
  * - empty()
  * - size()
  * - max_size()
  * - reserve(size_type new_cap)
  * - capacity()
  * - shrink_to_fit()
  *
  * ---- Modifiers ----
  * - clear()
  * - insert(const_iterator pos, const T& value)
  * - insert(const_iterator pos, T&& value)
  * - insert(const_iterator pos, size_type count, const T& value)
  * - insert(const_iterator pos, InputIt first, InputIt last)
  * - insert(const_iterator pos, std::initializer_list<T>)
  * - insert_range(const_iterator pos, R&& range)  (C++23)
  * - emplace(const_iterator pos, Args&&...)
  * - erase(const_iterator pos)
  * - erase(const_iterator first, const_iterator last)
  * - push_back(const T& value)
  * - push_back(T&& value)
  * - emplace_back(Args&&...)
  * - append_range(R&& range)                    (C++23)
  * - pop_back()
  * - resize(size_type count)
  * - resize(size_type count, const T& value)
  * - swap(vector& other)
  *
  * ---- Non-member functions ----
  * - operator==
  * - operator<=>                                (C++20; replaces !=, <, <=,
 >, >=)
  * - std::swap(vector&, vector&)
  * - std::erase(vector&, const U& value)        (C++20)
  * - std::erase_if(vector&, Pred)               (C++20)
  */
