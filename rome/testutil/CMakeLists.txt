add_library(testutil INTERFACE)
target_include_directories(testutil INTERFACE include)
target_link_libraries(testutil INTERFACE GTest::gmock util)

add_subdirectory(tests)
