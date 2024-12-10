add_library(
  soir_agent

  agent.cc
  controller_watcher.cc
  file_watcher.cc
  subscriber.cc
)

target_link_libraries(
  soir_agent

  efsw::efsw
  libremidi
  soir_proto
  soir_utils
  ${_GRPC_GRPCPP_UNSECURE}
)
