add_library(maethstro_midi
  midi.cc
)

target_link_libraries(maethstro_midi
  maethstro_proto
  maethstro_common
)
