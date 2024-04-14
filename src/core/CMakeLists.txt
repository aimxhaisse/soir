file(READ rt/engine.py INIT_ENGINE_PY)

configure_file(rt/init.cc.in rt/init.cc @ONLY)

add_library(neon_core
  neon.cc

  dsp/adsr.cc
  dsp/audio_buffer.cc
  dsp/engine.cc
  dsp/http.cc
  dsp/http_stream.cc
  dsp/mono_sampler.cc
  dsp/sample_manager.cc
  dsp/sample_pack.cc
  dsp/track.cc
  dsp/vorbis_encoder.cc

  rt/bindings.cc
  rt/engine.cc
  rt/init.cc
  rt/notifier.cc
)

target_link_libraries(neon_core
  AudioFile
  httplib::httplib
  libremidi
  neon_proto
  neon_utils
  ogg
  vorbis
  vorbisenc
  pybind11_headers
  pybind11::embed
)

add_executable(neon_core_test
  tests/base.cc
  tests/notification_recorder.cc

  tests/test_basic_log.cc
  tests/test_basic_loop.cc
  tests/test_get_bpm.cc
  tests/test_setup_tracks.cc
)

target_link_libraries(
  neon_core_test
  neon_core
  GTest::gtest_main
)
