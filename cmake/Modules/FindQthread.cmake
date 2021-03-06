find_package(PkgConfig)
pkg_check_modules(PC_QTHREAD QUIET qthread)

find_path(QTHREAD_INCLUDE_DIR qthread/qthread.hpp
  HINTS
  ${QTHREAD_ROOT} ENV QTHREAD_ROOT
  ${PC_QTHREAD_MINIMAL_INCLUDEDIR}
  ${PC_QTHREAD_MINIMAL_INCLUDE_DIRS}
  ${PC_QTHREAD_INCLUDEDIR}
  ${PC_QTHREAD_INCLUDE_DIRS}
  PATH_SUFFIXES include)

find_library(QTHREAD_LIBRARY NAMES qthread libqthread
  HINTS
  ${QTHREAD_ROOT} ENV QTHREAD_ROOT
  ${PC_QTHREAD_MINIMAL_LIBDIR}
  ${PC_QTHREAD_MINIMAL_LIBRARY_DIRS}
  ${PC_QTHREAD_LIBDIR}
  ${PC_QTHREAD_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64)

set(QTHREAD_INCLUDE_DIRS ${QTHREAD_INCLUDE_DIR})
set(QTHREAD_LIBRARIES ${QTHREAD_LIBRARY})

find_package_handle_standard_args(
  Qthread DEFAULT_MSG QTHREAD_INCLUDE_DIR QTHREAD_LIBRARY)

get_property(_type CACHE QTHREAD_ROOT PROPERTY TYPE)
if(_type)
  set_property(CACHE QTHREAD_ROOT PROPERTY ADVANCED 1)
  if("x${_type}" STREQUAL "xUNINITIALIZED")
    set_property(CACHE QTHREAD_ROOT PROPERTY TYPE PATH)
  endif()
endif()

mark_as_advanced(QTHREAD_ROOT QTHREAD_INCLUDE_DIR QTHREAD_LIBRARY)
