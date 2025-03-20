# Order is important here and is what is used by the C++ engine to
# load properly modules.
#
# Modules can only depend on modules from the upper level.

add_library(
  soir_core

  soir.cc
  parameter.cc

  engine/adsr.cc
  engine/audio_buffer.cc
  engine/audio_output.cc
  engine/controls.cc
  engine/engine.cc
  engine/fx_chorus.cc
  engine/fx_reverb.cc
  engine/fx_stack.cc
  engine/http.cc
  engine/http_stream.cc
  engine/lfo.cc
  engine/midi_ext.cc
  engine/sample.cc
  engine/sampler.cc
  engine/sample_manager.cc
  engine/sample_pack.cc
  engine/tools.cc
  engine/track.cc
  engine/midi_stack.cc
  engine/vorbis_encoder.cc

  rt/bindings.cc
  rt/engine.cc
  rt/notifier.cc
)

target_link_libraries(
  soir_core

  AudioFile
  libremidi
  Ogg::ogg
  protobuf::libprotobuf
  SDL2::SDL2
  Vorbis::vorbis
  Vorbis::vorbisenc
  pybind11::embed
  pybind11::headers
  rapidjson
  
  soir_proto
  soir_utils
)

add_executable(
  soir_core_test

  tests/base.cc
  tests/notification_recorder.cc

  tests/test_basic_live.cc
  tests/test_basic_log.cc
  tests/test_basic_loop.cc
  tests/test_controls.cc
  tests/test_get_bpm.cc
  tests/test_lfo.cc
  tests/test_setup_tracks.cc
  tests/test_exit.cc
  tests/test_parameter.cc
  tests/test_tools.cc
)

target_link_libraries(
  soir_core_test

  soir_core

  GTest::gtest_main
)
