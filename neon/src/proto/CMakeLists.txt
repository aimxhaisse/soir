# N E O N

set(PROTO_FILES neon.proto)

set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

add_library(neon_proto neon.proto)
target_link_libraries(neon_proto
    PUBLIC
        grpc
        grpc++
        protobuf::libprotobuf
        protobuf::libprotoc
)
target_include_directories(neon_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET neon_proto LANGUAGE cpp)
protobuf_generate(TARGET neon_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}")
