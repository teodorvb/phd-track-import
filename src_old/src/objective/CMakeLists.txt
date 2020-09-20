add_library(track-select-objective
  classify.cpp
  ffnn_classify.cpp
  hmm.cpp
  test_function.cpp
  common.cpp)

target_link_libraries(track-select-objective
  track-select-data
  ${TrackSelect_LIBRARIES})
set_target_properties(track-select-objective PROPERTIES COMPILE_FLAGS "-std=c++11")

