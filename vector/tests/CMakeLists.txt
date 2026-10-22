include(FetchContent)

message("Fetching gtest")
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

message("Making gtest available")
FetchContent_MakeAvailable(googletest)
message("Gtest available")

add_executable(vector_test vector_test.cpp)
target_link_libraries(vector_test PRIVATE
    vector
    GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(vector_test)
