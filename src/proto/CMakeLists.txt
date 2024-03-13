# Maethstro L I V E Proto.

set(PROTO_FILES live.proto)

set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

add_library(maethstro_proto live.proto)
target_link_libraries(maethstro_proto
    PUBLIC
        grpc
        grpc++
        protobuf::libprotobuf
        protobuf::libprotoc
)
target_include_directories(maethstro_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET maethstro_proto LANGUAGE cpp)
protobuf_generate(TARGET maethstro_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}")
