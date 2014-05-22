# (C) Copyright 2008 Andrew Sutton
# modified by Thomas Rehn to search for gmpxx
# Distributed under the Boost Software License, Version 1.0.

# ${GMP_LIBRARIES} contains libgmp and libgmpxx if GMP is found


FIND_PATH(GMP_INCLUDE_DIR NAMES gmp.h gmpxx.h)
FIND_LIBRARY(GMP_LIBRARY gmp)
FIND_LIBRARY(GMP_LIBRARY_CPP gmpxx)
SET(GMP_LIBRARIES ${GMP_LIBRARY} ${GMP_LIBRARY_CPP})

IF (GMP_INCLUDE_DIR AND GMP_LIBRARY)
   SET(GMP_FOUND TRUE)
ENDIF (GMP_INCLUDE_DIR AND GMP_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP  DEFAULT_MSG
                                  GMP_INCLUDE_DIR GMP_LIBRARIES)
