find_package(Boost 1.34.1 REQUIRED unit_test_framework)

include_directories(../sympol)

set(executables test-rank)

foreach(_exec ${executables})
  add_executable (${_exec} "${_exec}.cpp")
  add_test(${_exec} ${_exec})
  target_link_libraries(${_exec} ${Boost_LIBRARIES} ${GMP_LIBRARIES})
endforeach(_exec)
