add_library(maethstro_midi
  midi.cc
  engine.cc
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
