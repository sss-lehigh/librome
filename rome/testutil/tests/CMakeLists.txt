add_executable(fake_clock_test fake_clock_test.cc)
target_link_libraries(fake_clock_test PRIVATE
    testutil GTest::gtest GTest::gtest_main)
add_test(NAME fake_clock_test COMMAND fake_clock_test)
