find_package(PkgConfig)
pkg_check_modules(PC_JEMALLOC QUIET jemalloc)

find_path(JEMALLOC_INCLUDE_DIR jemalloc/jemalloc.h
  HINTS
  ${JEMALLOC_ROOT} ENV JEMALLOC_ROOT
  ${PC_JEMALLOC_MINIMAL_INCLUDEDIR}
  ${PC_JEMALLOC_MINIMAL_INCLUDE_DIRS}
  ${PC_JEMALLOC_INCLUDEDIR}
  ${PC_JEMALLOC_INCLUDE_DIRS}
  PATH_SUFFIXES include)

find_library(JEMALLOC_LIBRARY NAMES jemalloc libjemalloc
  HINTS
  ${JEMALLOC_ROOT} ENV JEMALLOC_ROOT
  ${PC_JEMALLOC_MINIMAL_LIBDIR}
  ${PC_JEMALLOC_MINIMAL_LIBRARY_DIRS}
  ${PC_JEMALLOC_LIBDIR}
  ${PC_JEMALLOC_LIBRARY_DIRS}
  PATH_SUFFIXES lib lib64)

set(JEMALLOC_INCLUDE_DIRS ${JEMALLOC_INCLUDE_DIR})
set(JEMALLOC_LIBRARIES ${JEMALLOC_LIBRARY})

find_package_handle_standard_args(
  Jemalloc DEFAULT_MSG JEMALLOC_INCLUDE_DIR JEMALLOC_LIBRARY)

get_property(_type CACHE JEMALLOC_ROOT PROPERTY TYPE)
if(_type)
  set_property(CACHE JEMALLOC_ROOT PROPERTY ADVANCED 1)
  if("x${_type}" STREQUAL "xUNINITIALIZED")
    set_property(CACHE JEMALLOC_ROOT PROPERTY TYPE PATH)
  endif()
endif()

mark_as_advanced(JEMALLOC_ROOT JEMALLOC_INCLUDE_DIR JEMALLOC_LIBRARY)