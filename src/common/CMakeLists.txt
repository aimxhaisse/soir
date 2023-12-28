add_library(maethstro_common
  config.cc
  signal.cc
  utils.cc
)

target_link_libraries(maethstro_common
  ${YAML_CPP_LIBRARIES}
  absl::strings
  absl::log
  absl::status  
  absl::statusor
)

add_executable(maethstro_common_test
  config_test.cc
)

target_link_libraries(
  maethstro_common_test
  maethstro_common
  GTest::gtest_main
)
