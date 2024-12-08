# S O I R

set(PROTO_FILES soir.proto)

set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

add_library(soir_proto soir.proto)
target_link_libraries(soir_proto
    PUBLIC
        grpc
        grpc++
        protobuf::libprotobuf
        protobuf::libprotoc
)
target_include_directories(soir_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET soir_proto LANGUAGE cpp)
protobuf_generate(TARGET soir_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}")
