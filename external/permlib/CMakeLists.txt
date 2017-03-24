cmake_minimum_required (VERSION 2.6)
project (PermLib)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(UseDoxygen OPTIONAL)

SET (Boost_FIND_REQUIRED TRUE)
SET (Boost_FIND_QUIETLY TRUE)
SET (Boost_USE_MULTITHREADED TRUE)
find_package(Boost 1.34.1 REQUIRED)

IF( NOT CMAKE_BUILD_TYPE )
    SET( CMAKE_BUILD_TYPE Debug CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
             FORCE )
ENDIF( NOT CMAKE_BUILD_TYPE )

add_definitions(-Wall -Woverloaded-virtual -ansi -pedantic -Wshadow)
include_directories(${Boost_INCLUDE_DIRS})
include_directories("${CMAKE_SOURCE_DIR}/include")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -DBOOST_DISABLE_ASSERTS")

# add -DPERMLIB_DEBUG_OUTPUT flag for full debug output
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DPERMLIB_DEBUGMODE")


enable_testing()

add_subdirectory(example)
add_subdirectory(test)

exec_program( ${CMAKE_COMMAND} ARGS -E copy_directory ${PROJECT_SOURCE_DIR}/data ${PROJECT_BINARY_DIR}/data )
