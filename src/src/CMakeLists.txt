add_library(track-import
  io.cpp
  helpers.cpp
  )

set_target_properties(track-import PROPERTIES COMPILE_FLAGS "-std=c++11" )
