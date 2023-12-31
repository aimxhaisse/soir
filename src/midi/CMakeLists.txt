file(READ engine.py INIT_ENGINE_PY)

configure_file(init.cc.in init.cc @ONLY)

add_library(maethstro_midi
  bindings.cc
  engine.cc
  init.cc
  midi.cc
  notifier.cc
)

target_link_libraries(maethstro_midi
  maethstro_proto
  maethstro_common
  pybind11_headers
  pybind11::embed
)

add_executable(maethstro_midi_test
  midi_test.cc
)

target_link_libraries(maethstro_midi_test
  maethstro_midi
  GTest::gtest_main
)
