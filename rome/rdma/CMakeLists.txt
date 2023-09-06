
add_library(rdma SHARED 
            src/rdma_device.cc
            src/rdma_broker.cc
            src/rdma_memory.cc)
target_include_directories(rdma PUBLIC include)
target_link_libraries(rdma PUBLIC logging util)
