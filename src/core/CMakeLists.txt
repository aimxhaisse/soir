# Order is important here and is what is used by the C++ engine to
# load properly modules.
#
# Modules can only depend on modules from the upper level.

add_library(
  soir_core

  soir.cc
  parameter.cc

  dsp/adsr.cc
  dsp/audio_buffer.cc
  dsp/audio_output.cc
  dsp/controls.cc
  dsp/engine.cc
  dsp/fx_chorus.cc
  dsp/fx_stack.cc
  dsp/http.cc
  dsp/http_stream.cc
  dsp/lfo.cc
  dsp/midi_ext.cc
  dsp/sample.cc
  dsp/sampler.cc
  dsp/sample_manager.cc
  dsp/sample_pack.cc
  dsp/tools.cc
  dsp/track.cc
  dsp/midi_stack.cc
  dsp/vorbis_encoder.cc

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
  tests/test_tools.cc
)

target_link_libraries(
  soir_core_test

  soir_core

  GTest::gtest_main
)
