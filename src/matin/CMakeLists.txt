add_library(maethstro_matin
  matin.cc
)

target_link_libraries(maethstro_matin
  maethstro_proto
  maethstro_common
)
