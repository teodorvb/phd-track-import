find_package(PkgConfig)

pkg_check_modules(PC_CPPUTIL QUIET c++util)
set(CPPUTIL_DEFINITIONS ${PC_CPPUTIL_CFLAGS_OTHER})


find_path(CPPUTIL_INCLUDE_DIR numutil++.hpp
          HINTS ${PC_CPPUTIL_INCLUDEDIR} ${PC_CPPUTIL_INCLUDE_DIRS}
          PATH_SUFFIXES cpputil)

find_library(CPPUTIL_LIBRARY NAMES c++util libc++util
             HINTS ${PC_CPPUTIL_LIBDIR} ${PC_CPPUTIL_LIBRARY_DIRS} )

set(CPPUTIL_LIBRARIES ${CPPUTIL_LIBRARY} )
set(CPPUTIL_INCLUDE_DIRS ${CPPUTIL_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CPPUTIL DEFAULT_MSG
                                  CPPUTIL_LIBRARY CPPUTIL_INCLUDE_DIR)

mark_as_advanced(CPPUTIL_INCLUDE_DIR CPPUTL_LIBRARY )
