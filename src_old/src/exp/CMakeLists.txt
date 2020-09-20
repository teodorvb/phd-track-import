add_library(track-select-exp
  experiment.cpp)
target_link_libraries(track-select-exp
  track-select-data
  ${LIBPQXX_LIBRARIES})
set_target_properties(track-select-exp PROPERTIES COMPILE_FLAGS "-std=c++11")