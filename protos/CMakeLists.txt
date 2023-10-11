set(Protobuf_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS metrics.proto colosseum.proto testutil.proto rdma.proto)  

add_library(protos SHARED ${PROTO_SRCS})
add_library(rome::protos ALIAS protos)
target_include_directories(protos PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
                            $<INSTALL_INTERFACE:include>)
target_link_libraries(protos PUBLIC protobuf::libprotobuf)

install(FILES ${PROTO_HDRS} DESTINATION include/protos)
