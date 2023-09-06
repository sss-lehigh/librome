
add_library(logging INTERFACE)
target_include_directories(logging INTERFACE include)
target_link_libraries(logging INTERFACE testutil)
