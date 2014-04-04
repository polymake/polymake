# (C) Copyright 2012 Thomas Rehn
# Distributed under the Boost Software License, Version 1.0.

# sets variables
#   ${BLISS_FOUND}       true if bliss was found
#   ${BLISS_INCLUDE_DIR}
#   ${BLISS_LIBRARIES}


FIND_PATH(BLISS_INCLUDE_DIR "bliss/graph.hh")
FIND_LIBRARY(BLISS_LIBRARY bliss)

SET(BLISS_LIBRARIES ${BLISS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bliss  DEFAULT_MSG
                                  BLISS_LIBRARY BLISS_INCLUDE_DIR)
