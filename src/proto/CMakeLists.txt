# S O I R

find_package(gRPC REQUIRED)

set(
  PROTO_FILES
  soir.proto
)

add_library(
  soir_proto
  ${PROTO_FILES}
)

target_link_libraries(
  soir_proto

  ${GRPC_INSTALL_DIR}/lib/libgrpc.a
  ${GRPC_INSTALL_DIR}/lib/libgrpc++.a
  ${PROTOBUF_INSTALL_DIR}/lib/libprotobuf.a
)

protobuf_generate(
  TARGET soir_proto
  LANGUAGE cpp
)

protobuf_generate(
  TARGET soir_proto
  LANGUAGE grpc
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)

target_include_directories(
  soir_proto PUBLIC ${Protobuf_INCLUDE_DIRS}
)
