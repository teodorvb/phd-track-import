add_library(track-select-simulate
  hmm.cpp)
target_link_libraries(track-select-simulate
  ${MSMM_LIBRARIES})
set_target_properties(track-select-simulate PROPERTIES COMPILE_FLAGS "-std=c++11")
