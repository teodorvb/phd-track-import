include(CTest)
set (FIXTURE_PATH ${CMAKE_SOURCE_DIR}/test/fixtures)

add_library(track-select-test SHARED
  fixtures.cpp)

target_link_libraries(track-select-test
  ${PQXX_LIBRARIES}
  ${Boost_LIBRARIES}
  boost_system
  boost_filesystem)
set_target_properties(track-select-test PROPERTIES COMPILE_FLAGS --std=c++11)

set(tests
  feature_vector_test.cpp
  sequence_test.cpp
  data_set_test.cpp
  algorithm_test.cpp)

foreach(test ${tests})
  get_filename_component(t_name ${test} NAME_WE)

  add_executable(${t_name} ${test})
  target_link_libraries(${t_name}
    track-select-test
    track-select-data)
  set_target_properties(${t_name} PROPERTIES COMPILE_FLAGS --std=c++11)
  add_test(NAME ${t_name} COMMAND ${t_name} ${FIXTURE_PATH})

endforeach()
