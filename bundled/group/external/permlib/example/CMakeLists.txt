include_directories(../include)

set(executables api-example example example-coset example-orbit example-smallgeneratingset)

foreach(_exec ${executables})
  add_executable ("${_exec}" "${_exec}.cpp")
endforeach(_exec)
