add_executable(design-scores design-scores.cpp)
target_link_libraries(design-scores
  track-select-data
  track-select-objective
  track-select-plot
  track-select-import
  track-select-tools
  ${ROOT_LIBRARIES}
  /opt/root/lib/libGviz3d.so
  /opt/root/lib/libRGL.so
  /opt/root/lib/libEve.so)
set_target_properties(design-scores PROPERTIES COMPILE_FLAGS "-std=c++11")

add_executable(gd gd.cpp)
target_link_libraries(gd
  track-select-data
  track-select-objective
  track-select-plot
  track-select-import
  track-select-tools
  ${ROOT_LIBRARIES}
  /opt/root/lib/libGviz3d.so
  /opt/root/lib/libRGL.so
  /opt/root/lib/libEve.so)
set_target_properties(gd PROPERTIES COMPILE_FLAGS "-std=c++11")

add_executable(level-detection level-detection.cpp)
target_link_libraries(level-detection
  track-select-data
  track-select-objective
  track-select-plot
  track-select-import
  track-select-tools
  ${ROOT_LIBRARIES}
  /opt/root/lib/libGviz3d.so
  /opt/root/lib/libRGL.so
  /opt/root/lib/libEve.so)
set_target_properties(level-detection PROPERTIES COMPILE_FLAGS "-std=c++11")