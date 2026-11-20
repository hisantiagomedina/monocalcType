cmake_minimum_required(VERSION 3.10)
project(monocalcType VERSION 0.1.0 LANGUAGES CXX)

# C++11 strict, no extensions
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Optional sanitizers (off by default on Windows)
option(MONOCALC_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
if(MONOCALC_SANITIZERS)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)
    check_cxx_compiler_flag("-fsanitize=undefined" HAS_UBSAN)
    if(HAS_ASAN AND HAS_UBSAN)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# Library sources
set(MONOCALC_SOURCES
    src/monocalc/lexer.cpp
    src/monocalc/parser.cpp
    src/monocalc/ast.cpp
    src/monocalc/evaluator.cpp
    src/monocalc/differentiator.cpp
    src/monocalc/integrator.cpp
    src/monocalc/domain.cpp
    src/monocalc/limiter.cpp
    src/monocalc/type.cpp
)

add_library(monocalc STATIC ${MONOCALC_SOURCES})
target_include_directories(monocalc PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Tests
enable_testing()
add_subdirectory(tests)
