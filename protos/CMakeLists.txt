set(Protobuf_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS metrics.proto colosseum.proto testutil.proto)  

add_library(protos SHARED ${PROTO_SRCS})
target_include_directories(protos PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/..")
target_link_libraries(protos PUBLIC protobuf::libprotobuf)
