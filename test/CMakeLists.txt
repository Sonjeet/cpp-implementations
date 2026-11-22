add_executable(unit_tests)

set(TEST_SOURCES
    vector_test.cpp
    unique_ptr_test.cpp)

set(DEBUG_TEST_SOURCES unique_ptr_test.cpp)

find_package(GTest)
target_sources(unit_tests PRIVATE ${TEST_SOURCES})
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
