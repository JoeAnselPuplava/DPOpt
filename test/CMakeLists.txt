
# Include directories
include_directories(
    /usr/local/include       # for libpqxx headers
    /opt/homebrew/opt/libpq/include  # for libpq headers
)
# Libraries
set(PQXX_LIBRARY /usr/local/lib/libpqxx.a)
set(PQ_LIBRARY /opt/homebrew/opt/libpq/lib/libpq.dylib)
link_directories(/usr/local/lib)

#Testing macro
macro(add_test_executable_with_lib _name _libs)
    add_executable(test_${_name} "${_name}.cpp")
    target_link_libraries(test_${_name} ${EMP-OT_LIBRARIES} ${_libs})
endmacro()

macro (add_test_case _name)
	add_test_executable_with_lib(${_name} "")
  	add_test(NAME ${_name} COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_${_name}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
endmacro()

macro(add_test_case_with_run _name)
    add_test_executable_with_lib(${_name} "${PQXX_LIBRARY};${PQ_LIBRARY}")
    add_test(NAME ${_name} COMMAND "./run" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_${_name}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
endmacro()


# Test cases
#add_test_case_with_run(bit)
#add_test_case_with_run(int)
IF(${ENABLE_FLOAT})
ENDIF(${ENABLE_FLOAT})
# add_test_case_with_run(circuit_file)
# add_test_case_with_run(example)
# add_test_case_with_run(oblisort)
# add_test_case_with_run(oblisort_perf)
# add_test_case_with_run(compaction)
# add_test_case_with_run(relsort)
# add_test_case_with_run(filter)
# add_test_case_with_run(pac_filter)
# add_test_case_with_run(equijoin)
# add_test_case_with_run(index_equijoin)
# add_test_case_with_run(qplan)
add_test_case_with_run(joetest)
add_test_case_with_run(joetest2)
add_test_case_with_run(template1)
add_test_case_with_run(template2)
add_test_case_with_run(template3)
add_test_case_with_run(template4)
add_test_case_with_run(template1Test)
add_test_case_with_run(template2Test)
add_test_case_with_run(template3Test)
add_test_case_with_run(template4Test)

# target_link_libraries(test_joetest ${PQXX_LIBRARY} ${PQ_LIBRARY})


