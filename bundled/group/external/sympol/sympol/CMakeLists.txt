INCLUDE (CheckSymbolExists)
INCLUDE (CheckTypeSize)
INCLUDE (CheckIncludeFile)

# Version number
SET(SYMPOL_MAJOR 0)
SET(SYMPOL_MINOR 1)
SET(SYMPOL_PATCH 8)
SET(SYMPOL_VERSION ${SYMPOL_MAJOR}.${SYMPOL_MINOR}.${SYMPOL_PATCH})

MESSAGE(STATUS "Building SymPol version ${SYMPOL_VERSION}")

CHECK_SYMBOL_EXISTS(sysconf "stdlib.h;unistd.h" HAVE_SYSCONF_PROTO)
CHECK_SYMBOL_EXISTS(getrusage "sys/resource.h" HAVE_GETRUSAGE_PROTO) 
CHECK_TYPE_SIZE(uint UINT)
CHECK_TYPE_SIZE(ulong ULONG)

find_package(Eigen3 3.0)
IF(EIGEN3_FOUND)
  include_directories(${EIGEN3_INCLUDE_DIR})
  set(HAVE_EIGEN 1)
ENDIF(EIGEN3_FOUND)

find_package(Bliss)
IF(BLISS_FOUND)
  include_directories(${BLISS_INCLUDE_DIR})
  set(TARGET_LIBS ${TARGET_LIBS} ${BLISS_LIBRARIES})
  set(HAVE_BLISS 1)
ENDIF(BLISS_FOUND)


CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)


SET (Boost_FIND_REQUIRED TRUE)
SET (Boost_FIND_QUIETLY TRUE)
SET (Boost_USE_MULTITHREADED TRUE)
find_package(Boost 1.34.1 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(../external/boost ../external/permlib ../external/lrslib-042c ../external/cddlib-094f ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-Woverloaded-virtual)
# lrs flags
add_definitions(-DGMP -DLRS_QUIET)
# cdd flags
add_definitions(-DGMPRATIONAL)


add_library(sympol SHARED 
  configuration.cpp
  polyhedronio.cpp 
  polyhedron.cpp
  polyhedrondatastorage.cpp
  qarray.cpp
  raycomputationlrs.cpp
  raycomputationcdd.cpp
  recursionstrategy.cpp
  symmetrycomputation.cpp
  symmetrycomputationdirect.cpp
  symmetrycomputationadm.cpp
  symmetrycomputationidm.cpp
  facesuptosymmetrylist.cpp
  symmetrygroupconstruction/matrixconstruction.cpp
  symmetrygroupconstruction/matrixconstructiondefault.cpp
  symmetrygroupconstruction/matrixconstructioneigen.cpp
  symmetrygroupconstruction/graphconstructiondefault.cpp
  symmetrygroupconstruction/graphconstructionbliss.cpp
  yal/reportlevel.cpp
  yal/usagestats.cpp
)
set_target_properties(sympol PROPERTIES VERSION ${SYMPOL_MAJOR}.${SYMPOL_MINOR}.${SYMPOL_PATCH} SOVERSION ${SYMPOL_MAJOR}.${SYMPOL_MINOR})

add_executable(sympolbin sympol.cpp)
set_target_properties(sympolbin PROPERTIES OUTPUT_NAME sympol)

set(TARGET_LIBS ${TARGET_LIBS} lrsgmp cddgmp ${GMP_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(sympol ${TARGET_LIBS})
target_link_libraries(sympolbin sympol)

if(INCLUDE_INSTALL_DIR)
else()
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()
if(LIB_INSTALL_DIR)
else()
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()
if(BIN_INSTALL_DIR)
else()
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()

install(TARGETS sympolbin sympol
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  common.h
  configuration.h
  facesuptosymmetrylist.h
  facewithdata.h
  matrix/algorithm.h
  matrix/invert.h
  matrix/matrix.h
  matrix/rank.h
  matrix/zmatrix.h
  polyhedrondatastorage.h
  polyhedron.h
  polyhedronio.h
  qarray.h
  raycomputationcdd.h
  raycomputation.h
  raycomputationlrs.h
  recursionstrategyadmidmlevel.h
  recursionstrategy.h
  recursionstrategyidmadm.h
  recursionstrategyidmadmlevel.h
  symmetrycomputationadm.h
  symmetrycomputationadmmemento.h
  symmetrycomputationdirect.h
  symmetrycomputation.h
  symmetrycomputationidm.h
  symmetrycomputationidmmemento.h
  symmetrycomputationmemento.h
  symmetrygroupconstruction/computesymmetries.h
  symmetrygroupconstruction/graphconstructionbliss.h
  symmetrygroupconstruction/graphconstructiondefault.h
  symmetrygroupconstruction/graphconstruction.h
  symmetrygroupconstruction/matrixconstructiondefault.h
  symmetrygroupconstruction/matrixconstructioneigen.h
  symmetrygroupconstruction/matrixconstruction.h
  types.h
  yal/logger.h
  yal/reportlevel.h
  yal/usagestats.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/sympol)
install(FILES ../man/sympol.1
        DESTINATION "share/man/man1")
