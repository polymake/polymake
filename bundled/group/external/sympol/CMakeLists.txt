cmake_minimum_required (VERSION 2.6)
project (SymPol)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/cmake/Modules")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

FIND_PACKAGE(GMP REQUIRED)
include_directories(${GMP_INCLUDE_DIR})

IF( NOT CMAKE_BUILD_TYPE )
    SET( CMAKE_BUILD_TYPE Debug CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
             FORCE )
ENDIF( NOT CMAKE_BUILD_TYPE )

add_definitions(-Wall)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -DBOOST_DISABLE_ASSERTS -DEIGEN_NO_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DBOOST_DISABLE_ASSERTS -DEIGEN_NO_DEBUG")

enable_testing()
add_test(NAME perltest WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib COMMAND perl test-sympol.pl ${PROJECT_BINARY_DIR})

add_subdirectory(sympol)
add_subdirectory(external/lrslib-042c)
add_subdirectory(external/cddlib-094f)
add_subdirectory(test)

exec_program( ${CMAKE_COMMAND} ARGS -E copy_directory ${PROJECT_SOURCE_DIR}/data ${PROJECT_BINARY_DIR}/data )

