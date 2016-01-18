# libnormaliz source files
SET(sources cone_property.cpp HilbertSeries.cpp libnormaliz-templated.cpp cone_helper.cpp offload_handler.cpp bottom.cpp)
# libnormaliz header files
SET(headers cone.h cone_property.h convert.h general.h HilbertSeries.h integer.h libnormaliz.h map_operations.h matrix.h my_omp.h normaliz_exception.h sublattice_representation.h vector_operations.h version.h)

# add the necessary include directories
INCLUDE_DIRECTORIES(..)
FIND_PACKAGE(Boost REQUIRED)
#IF (Boost_FOUND)
#  MESSAGE(STATUS "Boost found with include dir: ${Boost_INCLUDE_DIR}")
#ENDIF()
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

SET (NMZ_BUILD_BOTH "NO" CACHE BOOL "build shared AND static libnormaliz")

IF (NMZ_BUILD_BOTH OR NOT NMZ_BUILD_STATIC)
  # create the shared library
  ADD_LIBRARY(normaliz-shared SHARED ${sources})
  SET_TARGET_PROPERTIES(normaliz-shared PROPERTIES OUTPUT_NAME normaliz VERSION ${VERSION} SOVERSION 0)
  # set the dependency on gmp
  TARGET_LINK_LIBRARIES (normaliz-shared ${GMPXX_LIBRARIES} ${GMP_LIBRARIES})
  INSTALL(TARGETS normaliz-shared
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    RUNTIME DESTINATION bin
    COMPONENT sharedlibs
  )
ENDIF ()

IF (NMZ_BUILD_BOTH OR NMZ_BUILD_STATIC)
  #create the static library
  ADD_LIBRARY(normaliz-static STATIC ${sources})
  SET_TARGET_PROPERTIES(normaliz-static PROPERTIES OUTPUT_NAME normaliz VERSION ${VERSION} SOVERSION 0)
  INSTALL(TARGETS normaliz-static ARCHIVE DESTINATION lib${LIB_SUFFIX})
ENDIF ()

INSTALL(FILES ${headers} DESTINATION include/${PROJECT_NAME} COMPONENT headers)

SET(LIBNORMALIZ_INCLUDE_DIR ${includedir})
SET(LIBNORMALIZ_INCLUDE_DIRS ${LIBNORMALIZ_INCLUDE_DIR})
SET(LIBNORMALIZ_ROOT_DIR ${prefix})
SET(LIBNORMALIZ_VERSION_STRING ${VERSION_STRING})
SET(LIBNORMALIZ_VERSION_MAJOR ${VERSION_MAJOR})
SET(LIBNORMALIZ_VERSION_MINOR ${VERSION_MINOR})
SET(LIBNORMALIZ_VERSION_PATCH ${VERSION_PATCH})
CONFIGURE_FILE(version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/version.h @ONLY)
SET(LIBNORMALIZ_USE_FILE ${CMAKE_INSTALL_PREFIX}/${LIBNORMALIZ_CMAKE_CONFIG_DIR}/UseLibnormaliz.cmake)
CONFIGURE_FILE(../LibnormalizConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/LibnormalizConfig.cmake @ONLY)
#INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/LibnormalizConfig.cmake ../UseLibnormaliz.cmake DESTINATION ${LIBNORMALIZ_CMAKE_CONFIG_DIR})

#ADD_SUBDIRECTORY(examples)
