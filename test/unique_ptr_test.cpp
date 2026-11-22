#include "unique_ptr.h"
#include <algorithm>
#include <gtest/gtest.h>

TEST(up_test, construct_up) {
    dev::unique_ptr<int> p1;
    EXPECT_EQ(p1.get(), nullptr);

    dev::unique_ptr<int> p2{new int(10)};
    EXPECT_EQ(*p2.get(), 10);
}

TEST(up_test, move_constructor) {
    dev::unique_ptr<int> p1{new int{10}};
    EXPECT_EQ(*p1.get(), 10);
    dev::unique_ptr<int> p2{std::move(p1)};
    EXPECT_EQ(p1.get(), nullptr);
    EXPECT_EQ(*p2.get(), 10);
}

TEST(up_test, bool_operator) {
    dev::unique_ptr<int> p{new int(10)};
    EXPECT_TRUE(p);
    dev::unique_ptr<int> p2;
    EXPECT_TRUE(!p2);
}

TEST(up_test, assignment_operator) {
    dev::unique_ptr<int> p1{new int(10)};
    dev::unique_ptr<int> p2;
    p2 = std::move(p1);

    EXPECT_EQ(p1.get(), nullptr);
    EXPECT_EQ(*p2.get(), 10);
}

TEST(up_test, reset_single_obj) {
    dev::unique_ptr<int> p1{new int(10)};
    p1.reset();
    EXPECT_EQ(p1.get(), nullptr);

    p1.reset(new int(10));
    EXPECT_EQ(*p1.get(), 10);
}

TEST(up_test, reset_array_obj) {
    std::size_t size = 10;
    dev::unique_ptr<int[]> p{new int[size]};
    EXPECT_TRUE(p);

    p.reset();
    EXPECT_FALSE(p);
    EXPECT_EQ(p.get(), nullptr);

    p.reset(new int[size]);
    EXPECT_TRUE(p);

    for (auto i = 0; i < size; i++) {
        p.get()[i] = i;
    }

    for (auto i = 0; i < size; i++) {
        EXPECT_EQ(p.get()[i], i);
    }
}

TEST(up_test, release_ownership) {
    int* ptr = nullptr;
    dev::unique_ptr<int> p{new int(10)};
    EXPECT_EQ(ptr, nullptr);
    auto temp_ptr = p.get();
    ptr = p.release();
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_EQ(ptr, temp_ptr);
}

TEST(up_test, swap) {
    dev::unique_ptr<int> p1{new int(10)};
    dev::unique_ptr<int> p2{new int(20)};

    using Pointers = std::pair<int*, int*>;
    Pointers original = {p1.get(), p2.get()};
    p1.swap(p2);
    EXPECT_EQ(p1.get(), original.second);
    EXPECT_EQ(p2.get(), original.first);

    std::swap(p1, p2);
    EXPECT_EQ(p1.get(), original.first);
    EXPECT_EQ(p2.get(), original.second);
}

TEST(up_test, dereferencing) {
    dev::unique_ptr<int> p{new int(10)};
    *p = 100;
    EXPECT_EQ(*p.get(), 100);

    *p = 0;
    EXPECT_EQ(*p.get(), 0);
}

TEST(up_test, pointer_access) {
    using Pointers = std::pair<int, int>;

    dev::unique_ptr<Pointers> p{new Pointers{10, 20}};
    EXPECT_EQ(p->first, 10);
    EXPECT_EQ(p->second, 20);

    p->first = 0;
    p->second = 0;
    EXPECT_EQ(p->first, 0);
    EXPECT_EQ(p->second, 0);
}

TEST(up_test, make_unique_single_obj) {
    auto ptr = dev::make_unique<int>(10);
    EXPECT_EQ(*ptr, 10);
}

TEST(up_test, make_unique_array_obj) {
    std::size_t size = 10;
    auto ptr = dev::make_unique<int[]>(size);
    for (auto i = 0; i < size; i++) {
        ptr[i] = i;
    }
    for (auto i = 0; i < size; i++) {
        EXPECT_EQ(ptr[i], i);
    }
}
