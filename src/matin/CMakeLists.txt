add_library(maethstro_matin
  matin.cc
  subscriber.cc
  file_watcher.cc
)

target_link_libraries(maethstro_matin
  maethstro_proto
  maethstro_common
  efsw
)

add_executable(maethstro_matin_test
  file_watcher_test.cc
)

target_link_libraries(
  maethstro_matin_test
  maethstro_matin
  GTest::gtest_main
)
