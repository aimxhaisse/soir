add_library(soir_agent
  agent.cc
  controller_watcher.cc
  file_watcher.cc
  subscriber.cc
)

target_link_libraries(soir_agent
  ${EFSW_INSTALL_DIR}/lib/libefsw.a
  ${LIBREMIDI_INSTALL_DIR}/lib/libremidi.a
  ${GRPC_INSTALL_DIR}/lib/libgrpc.a
  ${GRPC_INSTALL_DIR}/lib/libgrpc++.a
  soir_proto
  soir_utils
)

add_dependencies(
  soir_agent

  efsw
  libremidi
  grpc
)
