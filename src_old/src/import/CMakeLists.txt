add_library(track-select-import
  minist.cpp
  smi.cpp)
target_link_libraries(track-select-import
  ${MSMM_LIBRARIES})
set_target_properties(track-select-import PROPERTIES COMPILE_FLAGS "-std=c++11")
