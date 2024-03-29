add_library(neon_agent
  agent.cc
  controller_watcher.cc
  file_watcher.cc
  subscriber.cc
)

target_link_libraries(neon_agent
  efsw
  libremidi
  neon_proto
  neon_utils
)
