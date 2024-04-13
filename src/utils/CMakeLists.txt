add_library(neon_utils
  config.cc
  signal.cc
  misc.cc
)

target_link_libraries(neon_utils
  ${YAML_CPP_LIBRARIES}
  absl::strings
  absl::log
  absl::status  
  absl::statusor
)

add_executable(neon_utils_test
  tests/config_test.cc
)

target_link_libraries(
  neon_utils_test
  neon_utils
  GTest::gtest_main
)
