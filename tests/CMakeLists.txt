# Basic compilation test
add_executable(test_compile test_compile.cpp)
target_link_libraries(test_compile PRIVATE monocalc)
add_test(NAME test_compile COMMAND test_compile)
