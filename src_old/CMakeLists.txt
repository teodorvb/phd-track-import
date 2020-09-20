cmake_minimum_required(VERSION 2.8)
project(track-select-tools)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -g -fopenmp" )

set(BUILD_SHARED_LIBS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules") 
list(APPEND CMAKE_MODULE_PATH "$ENV{ROOTSYS}/etc/cmake/")


find_package(TrackSelect 0.1 REQUIRED)
find_package(LIBPQXX 4.0)
find_package(MSMM REQUIRED)
find_package(Boost COMPONENTS system program_options iostreams filesystem REQUIRED)

#find_package(Qt4 REQUIRED QtGui QtCore)
#find_package(MathGL2 2.1 REQUIRED Qt)
#find_package(CUDA REQUIRED)
#find_package(ROOT REQUIRED)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${TrackSelect_INCLUDE_DIRS}
  ${MSMM_INCLUDE_DIRS}
  ${QT_INCLUDE_DIR}
  ${MATHGL2_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR})
add_subdirectory(src)
add_subdirectory(exp)
add_subdirectory(tools)
add_subdirectory(test)
#add_subdirectory(desktop)

enable_testing()
