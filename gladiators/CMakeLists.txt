add_executable(coroutines_out coroutines/main.cc)
target_link_libraries(coroutines_out PRIVATE rome::rome)

add_executable(hello_world_out hello_world/main.cc)
target_link_libraries(hello_world_out PRIVATE rome::rome)
target_link_libraries(hello_world_out PRIVATE absl::flags absl::flags_parse)
