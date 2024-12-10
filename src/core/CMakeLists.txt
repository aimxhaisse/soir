# Order is important here and is what is used by the C++ engine to
# load properly modules.
#
# Modules can only depend on modules from the upper level.

add_library(
  soir_core

  soir.cc

  dsp/adsr.cc
  dsp/audio_buffer.cc
  dsp/audio_output.cc
  dsp/engine.cc
  dsp/http.cc
  dsp/http_stream.cc
  dsp/midi_ext.cc
  dsp/sampler.cc
  dsp/sample_manager.cc
  dsp/sample_pack.cc
  dsp/track.cc
  dsp/vorbis_encoder.cc
  dsp/audio_output.cc
  dsp/midi_stack.cc

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
  tests/test_get_bpm.cc
  tests/test_setup_tracks.cc
  tests/test_exit.cc
)

target_link_libraries(
  soir_core_test

  soir_core

  GTest::gtest_main
)
