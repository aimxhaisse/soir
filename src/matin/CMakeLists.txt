add_library(maethstro_matin
  matin.cc
  controller_watcher.cc
  subscriber.cc
  file_watcher.cc
  utils.cc
)

target_link_libraries(maethstro_matin
  efsw
  libremidi
  maethstro_proto
  maethstro_common
)

add_executable(maethstro_matin_test
  matin_test.cc
  file_watcher_test.cc
)

target_link_libraries(
  maethstro_matin_test
  maethstro_matin
  maethstro_common
  maethstro_midi
  GTest::gtest_main
)
