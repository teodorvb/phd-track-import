find_package(PkgConfig)
find_package(CPPUTIL REQUIRED)
find_package(LAPACK REQUIRED)
find_package(Boost COMPONENTS system)

pkg_check_modules(PC_MSMM QUIET msmm-common)
set(MSMM_DEFINITIONS ${PC_MSMM_CFLAGS_OTHER})

pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0 glibmm-2.4)


find_path(MSMM_INCLUDE_DIR datamodel.hpp
          HINTS ${PC_MSMM_INCLUDEDIR} ${PC_MSMM_INCLUDE_DIRS}
          PATH_SUFFIXES msmm-common )

find_library(MSMM_LIBRARY NAMES msmm-common libmsmm-common
             HINTS ${PC_MSMM_LIBDIR} ${PC_MSMM_LIBRARY_DIRS} )

set(MSMM_LIBRARIES 
  ${MSMM_LIBRARY} 
  ${CPPUTIL_LIBRARIES}
  ${LAPACK_LIBRARIES}
  ${Boost_LIBRARIES})


set(MSMM_INCLUDE_DIRS 
  ${CPPUTIL_INCLUDE_DIR} 
  ${MSMM_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${LAPACK_INCLUDE_DIR}
  ${GLIB_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(MSMM DEFAULT_MSG
                                  MSMM_LIBRARY MSMM_INCLUDE_DIR)

mark_as_advanced(MSMM_INCLUDE_DIR MSMM_LIBRARY )

