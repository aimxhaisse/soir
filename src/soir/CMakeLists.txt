add_library(maethstro_soir
  audio_buffer.cc
  soir.cc
  engine.cc
  http.cc
  http_stream.cc
  vorbis_encoder.cc
)

target_link_libraries(maethstro_soir
  maethstro_proto
  maethstro_common
  httplib::httplib
  vorbis
  vorbisenc
  ogg
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
