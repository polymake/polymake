find_package(Boost 1.34.1 REQUIRED unit_test_framework)

include_directories(../include)

set(executables test-base test-trans test-generators test-partition test-permutation 
    test-search-setstabilizer test-symmetric-group test-search-matrixauto test-orbit-lex-min 
    test-api test-test test-export test-abstract-groups)

foreach(_exec ${executables})
  add_executable (${_exec} "${_exec}.cpp")
  add_test(${_exec} ${_exec})
  target_link_libraries(${_exec} boost_unit_test_framework-mt)
endforeach(_exec)
