# Order is important here and is what is used by the C++ engine to
# load properly modules.
#
# Modules can only depend on modules from the upper level.

add_library(
  soir_core

  soir.cc
  parameter.cc

  adsr.cc
  audio_buffer.cc
  audio_output.cc
  controls.cc
  dsp/chorus.cc
  dsp/comb_filter.cc
  dsp/delay.cc
  dsp/delayed_apf.cc
  dsp/lfo.cc
  dsp/lpf.cc
  dsp/modulated_delay.cc
  dsp/tools.cc
  engine.cc
  fast_random.cc
  fx/fx_chorus.cc
  fx/fx_reverb.cc
  fx/fx_stack.cc
  http.cc
  http_stream.cc
  inst/midi_ext.cc
  inst/sampler.cc
  sample.cc
  sample_manager.cc
  sample_pack.cc
  tools.cc
  track.cc
  midi_stack.cc
  vorbis_encoder.cc

  rt/bindings.cc
  rt/runtime.cc
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
