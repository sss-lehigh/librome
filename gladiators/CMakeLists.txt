add_executable(coroutines coroutines/main.cc)
target_link_libraries(coroutines PRIVATE rome::rome)

add_executable(hello_world hello_world/main.cc)
target_link_libraries(hello_world PRIVATE rome::rome)
target_link_libraries(hello_world PRIVATE absl::flags absl::flags_parse)
