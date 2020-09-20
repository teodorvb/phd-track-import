add_library(track-select-tools
  timer.cpp
  command_line.cpp)
set_target_properties(track-select-tools
  PROPERTIES COMPILE_FLAGS "-std=c++11")