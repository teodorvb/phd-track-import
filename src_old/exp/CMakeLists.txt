# add_executable(reconstruct-minist reconstruct-minist.cpp)
# target_link_libraries(reconstruct-minist
#   ${TrackSelect_LIBRARIES}
#   ${LIBPQXX_LIBRARIES}
#   track-select-algorithm
#   track-select-data
#   track-select-exp
#   track-select-plot
#   )
# set_target_properties(reconstruct-minist PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(cross-validate cross-validate.cpp)
target_link_libraries(cross-validate
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  track-select-data
  track-select-exp
  track-select-objective
  track-select-algorithm
  track-select-tools
  )
set_target_properties(cross-validate
  PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(fitness-evolution fitness-evolution.cpp)
target_link_libraries(fitness-evolution
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  track-select-data
  track-select-exp
  track-select-objective
  track-select-algorithm
  )
set_target_properties(fitness-evolution
  PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(find-parameters find-parameters.cpp)
target_link_libraries(find-parameters
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  track-select-data
  track-select-exp
  track-select-objective
  track-select-algorithm
  )
set_target_properties(find-parameters
  PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(auto-encoder auto-encoder.cpp)
target_link_libraries(auto-encoder
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  track-select-data
  track-select-exp
  track-select-objective
  track-select-algorithm
  track-select-plot
  )
set_target_properties(auto-encoder
  PROPERTIES COMPILE_FLAGS "-std=c++11" )

