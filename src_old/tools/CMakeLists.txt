add_executable(import-minist import-minist.cpp)
target_link_libraries(import-minist
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  ${Boost_LIBRARIES}
  track-select-data
  track-select-import
  )
set_target_properties(import-minist PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(import-smi import-smi.cpp)
target_link_libraries(import-smi
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  ${Boost_LIBRARIES}
  track-select-data
  track-select-import
  track-select-tools
  )
set_target_properties(import-smi PROPERTIES COMPILE_FLAGS "-std=c++11" )

add_executable(compress compress.cpp)
target_link_libraries(compress
  ${TrackSelect_LIBRARIES}
  ${Boost_LIBRARIES}
  track-select-objective
  track-select-data)
set_target_properties(compress PROPERTIES COMPILE_FLAGS "-std=c++11")


add_executable(reduce-data-size reduce-data-size.cpp)
target_link_libraries(reduce-data-size
  ${TrackSelect_LIBRARIES}
  track-select-objective
  track-select-data)
set_target_properties(reduce-data-size PROPERTIES COMPILE_FLAGS "-std=c++11")


# add_executable(plot-data-set plot-data-set.cpp)
# target_link_libraries(plot-data-set
#   ${TrackSelect_LIBRARIES}
#   ${LIBPQXX_LIBRARIES}
#   track-select-data
#   track-select-import
#   track-select-plot
#   )
# set_target_properties(plot-data-set PROPERTIES COMPILE_FLAGS "-std=c++11" )


# add_executable(plot-sequence-histogram plot-sequence-histogram.cpp)
# target_link_libraries(plot-sequence-histogram
#   ${TrackSelect_LIBRARIES}
#   ${LIBPQXX_LIBRARIES}
#   track-select-data
#   )
# set_target_properties(plot-sequence-histogram PROPERTIES COMPILE_FLAGS "-std=c++11" )


# add_executable(scale scale.cpp)
# target_link_libraries(scale
#   ${TrackSelect_LIBRARIES}
#   ${LIBPQXX_LIBRARIES}
#   track-select-data
#   track-select-import
#   track-select-plot
#   )
# set_target_properties(scale PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(simulate simulate.cpp)
target_link_libraries(simulate
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  track-select-data
  track-select-import
  track-select-simulate
  track-select-plot
  track-select-tools
  ${Boost_LIBRARIES})
set_target_properties(simulate PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(correlate-bg correlate-bg.cpp)
target_link_libraries(correlate-bg
  ${TrackSelect_LIBRARIES}
  ${LIBPQXX_LIBRARIES}
  ${MSMM_LIBRARIES}
  ${Boost_LIBRARIES})
set_target_properties(correlate-bg PROPERTIES COMPILE_FLAGS "-std=c++11" )




