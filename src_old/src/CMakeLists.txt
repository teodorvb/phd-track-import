add_subdirectory(import)
add_subdirectory(data)
add_subdirectory(exp)
add_subdirectory(objective)
add_subdirectory(simulate)
add_subdirectory(tools)

add_library(track-select-algorithm
  algorithm.cpp)
target_link_libraries(track-select-algorithm
  ${Boost_LIBRARIES}
  )
set_target_properties(track-select-algorithm
  PROPERTIES COMPILE_FLAGS "-std=c++11" )

add_library(track-select-plot
  plot.cpp)
target_link_libraries(track-select-plot
  ${TrackSelect_LIBRARIES}
  ${Boost_LIBRARIES}
  ${ROOT_LIBRARIES})
set_target_properties(track-select-plot
  PROPERTIES COMPILE_FLAGS "-std=c++11 ${ROOT_CXX_FLAGS}" )

  
