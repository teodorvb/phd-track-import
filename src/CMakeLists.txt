cmake_minimum_required(VERSION 2.8)
project(track-import)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -g -fopenmp" )

set(BUILD_SHARED_LIBS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules") 
list(APPEND CMAKE_MODULE_PATH "$ENV{ROOTSYS}/etc/cmake/")

ADD_DEFINITIONS ( -DPQXX_HIDE_EXP_OPTIONAL )

find_package(LIBPQXX 6.4 EXACT)
find_package(MSMM REQUIRED)
find_package(HDF5 REQUIRED)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${HDF5_INCLUDE_DIRS}
  ${LIBPQXX_INCLUDE_DIRS}
  ${MSMM_INCLUDE_DIRS}
  )

add_subdirectory(src)

# Targets
# Import data from FLImP compatable to a database
add_executable(smi-db-import-classification smi-db-import-classification.cpp)
target_link_libraries(smi-db-import-classification
  ${LIBPQXX_LIBRARIES}
  ${MSMM_LIBRARIES}
  track-import
  )
set_target_properties(smi-db-import-classification PROPERTIES COMPILE_FLAGS "-std=c++11" )

add_executable(smi-db-import-level-detection smi-db-import-level-detection.cpp)
target_link_libraries(smi-db-import-level-detection
  ${LIBPQXX_LIBRARIES}
  ${MSMM_LIBRARIES}
  track-import
  )
set_target_properties(smi-db-import-level-detection PROPERTIES COMPILE_FLAGS "-std=c++11" )


add_executable(smi-csv-import-all-tracks smi-csv-import-all-tracks.cpp)
target_link_libraries(smi-csv-import-all-tracks
  ${LIBPQXX_LIBRARIES}
  ${MSMM_LIBRARIES}
  track-import
  )
set_target_properties(smi-csv-import-all-tracks PROPERTIES COMPILE_FLAGS "-std=c++11" )


enable_testing()
