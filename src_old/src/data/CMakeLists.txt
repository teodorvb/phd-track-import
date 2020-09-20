add_library(track-select-data
  storable.cpp
  foreign_key.cpp
  data_point.cpp
  feature_vector.cpp
  sequence.cpp
  data_set.cpp)

target_link_libraries(track-select-data
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES})
set_target_properties(track-select-data PROPERTIES COMPILE_FLAGS "-std=c++11")
