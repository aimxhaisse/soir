add_library(neon_core
  neon.cc

  dsp/adsr.cc
  dsp/audio_buffer.cc
  dsp/mono_sampler.cc
  dsp/engine.cc
  dsp/track.cc
  dsp/http.cc
  dsp/http_stream.cc
  dsp/vorbis_encoder.cc
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
