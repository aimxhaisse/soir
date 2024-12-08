add_library(soir_agent
  agent.cc
  controller_watcher.cc
  file_watcher.cc
  subscriber.cc
)

target_link_libraries(soir_agent
  efsw
  libremidi
  soir_proto
  soir_utils
)
