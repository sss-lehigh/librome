include(CMakeFindDependencyMacro)

find_dependency(spdlog REQUIRED) #defines spdlog::spdlog
find_dependency(absl REQUIRED) # defines absl::absl
find_dependency(Protobuf REQUIRED) # protobuf
find_dependency(Coroutines REQUIRED) # defines std::coroutines for linking
find_dependency(RDMA REQUIRED) # defines rdma::ibverbs and rdma::cm for linking

include(${CMAKE_CURRENT_LIST_DIR}/romeTargets.cmake)

