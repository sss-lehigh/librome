add_test_executable(qps_controller_test qps_controller_test.cc)
add_test_executable(streams_test streams/streams_test.cc)

message(WARNING "Workload driver test disabled")
add_test_executable(workload_driver_test workload_driver_test.cc DISABLE_TEST)
