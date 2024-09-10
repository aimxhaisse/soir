# Order is important here and is what is used by the C++ engine to
# load properly modules.
#
# Modules can only depend on modules from the upper level.

# Level 0
file(READ rt/py/neon/errors.py MOD0_ERRORS_PY)

# Level 1
file(READ rt/py/neon/internals.py MOD1_INTERNALS_PY)

# Level 2
file(READ rt/py/neon/sampler.py MOD2_SAMPLER_PY)
file(READ rt/py/neon/bpm.py MOD2_BPM_PY)
file(READ rt/py/neon/midi.py MOD2_MIDI_PY)

# Level 3
file(READ rt/py/neon/__init__.py MOD3_NEON_PY)

configure_file(rt/init.cc.in rt/init.cc @ONLY)

add_library(neon_core
  neon.cc

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
  rtaudio
  vorbis
  vorbisenc
  pybind11_headers
  pybind11::embed
)

add_executable(neon_core_test
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
  neon_core_test
  neon_core
  GTest::gtest_main
)
