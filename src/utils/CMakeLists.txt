add_library(
  soir_utils

  config.cc
  signal.cc
  misc.cc
)

target_link_libraries(
  soir_utils

  absl::strings
  absl::log
  absl::status
  absl::statusor
  yaml-cpp::yaml-cpp
)

add_executable(
  soir_utils_test

  tests/config_test.cc
)

target_link_libraries(
  soir_utils_test

  soir_utils

  GTest::gtest_main
)