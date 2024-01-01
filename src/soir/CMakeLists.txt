add_library(maethstro_soir
  soir.cc
  http.cc
  engine.cc
)

target_link_libraries(maethstro_soir
  maethstro_proto
  maethstro_common
  httplib::httplib
)

add_executable(maethstro_soir_test
  soir_test.cc
)

target_link_libraries(
  maethstro_soir_test
  maethstro_soir
  maethstro_common
  GTest::gtest_main
)
