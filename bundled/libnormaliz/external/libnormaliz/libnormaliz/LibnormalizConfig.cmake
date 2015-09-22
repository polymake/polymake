#                                               -*- cmake -*-
#
#  LibnormalizConfig.cmake(.in)
#
#  Copyright (C) 2014 Christof Soeger <csoeger@uos.de>
#


# Use the following variables to compile and link against libnormaliz:
#  LIBNORMALIZ_FOUND              - True if libnormaliz was found on your system
#  LIBNORMALIZ_USE_FILE           - The file making libnormaliz usable
#  LIBNORMALIZ_DEFINITIONS        - Definitions needed to build with libnormaliz
#  LIBNORMALIZ_INCLUDE_DIRS       - Directory where OT.hxx can be found
#  LIBNORMALIZ_INCLUDE_DIRS       - List of directories of libnormaliz and it's dependencies
#  LIBNORMALIZ_LIBRARY            - libnormaliz library location
#  LIBNORMALIZ_LIBRARIES          - List of libraries to link against libnormaliz library
#  LIBNORMALIZ_LIBRARY_DIRS       - List of directories containing libnormaliz' libraries
#  LIBNORMALIZ_ROOT_DIR           - The base directory of libnormaliz
#  LIBNORMALIZ_VERSION_STRING     - A human-readable string containing the version
#  LIBNORMALIZ_VERSION_MAJOR      - The major version of libnormaliz
#  LIBNORMALIZ_VERSION_MINOR      - The minor version of libnormaliz
#  LIBNORMALIZ_VERSION_PATCH      - The patch version of libnormaliz

set ( LIBNORMALIZ_FOUND 1 )
set ( LIBNORMALIZ_USE_FILE     "/usr/local//UseLibnormaliz.cmake" )

set ( LIBNORMALIZ_DEFINITIONS  "" )
set ( LIBNORMALIZ_INCLUDE_DIR  "/usr/local/include/libnormaliz" )
set ( LIBNORMALIZ_INCLUDE_DIRS "/usr/local/include/libnormaliz" )
set ( LIBNORMALIZ_LIBRARY      "/usr/local/lib/_LIBNORMALIZ_LIBRARY_LOCATION-NOTFOUND" )
set ( LIBNORMALIZ_LIBRARIES    "/usr/local/lib/_LIBNORMALIZ_LIBRARY_LOCATION-NOTFOUND" )
set ( LIBNORMALIZ_LIBRARY_DIRS "" )
set ( LIBNORMALIZ_ROOT_DIR     "/usr/local" )

set ( LIBNORMALIZ_VERSION_STRING "2.11.1" )
set ( LIBNORMALIZ_VERSION_MAJOR  "2" )
set ( LIBNORMALIZ_VERSION_MINOR  "11" )
set ( LIBNORMALIZ_VERSION_PATCH  "1" )
