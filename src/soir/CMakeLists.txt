add_library(maethstro_soir
  audio_buffer.cc
  soir.cc
  engine.cc
  track.cc
  http.cc
  http_stream.cc
  vorbis_encoder.cc
)

target_link_libraries(maethstro_soir
  AudioFile
  maethstro_proto
  maethstro_common
  httplib::httplib
  libremidi
  ogg
  vorbis
  vorbisenc
)

add_executable(maethstro_soir_test
  soir_test.cc
)

target_link_libraries(
  maethstro_soir_test
  maethstro_soir
  maethstro_common
  GTest::gtest_main
)
