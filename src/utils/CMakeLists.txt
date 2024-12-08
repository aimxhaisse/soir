add_library(soir_utils
  config.cc
  signal.cc
  misc.cc
)

target_link_libraries(
  soir_utils

  ${YAMLCPP_INSTALL_DIR}/lib/libyaml-cpp.a
  ${ABSL_INSTALL_DIR}/lib/libabsl_strings.a
  ${ABSL_INSTALL_DIR}/lib/libabsl_log.a
  ${ABSL_INSTALL_DIR}/lib/libabsl_status.a
  ${ABSL_INSTALL_DIR}/lib/libabsl_statusor.a
)

add_dependencies(
  soir_utils

  absl
  yaml-cpp
)

add_executable(soir_utils_test
  tests/config_test.cc
)

target_link_libraries(
  soir_utils_test

  soir_utils

  ${GOOGLETEST_INSTALL_DIR}/lib/libgoogletest.a
)

add_dependencies(
  soir_utils

  googletest
)
