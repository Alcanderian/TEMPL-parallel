cmake_minimum_required(VERSION 2.8.12)
project(Parallel)

if (CMAKE_C_COMPILER MATCHES "icc")
    MESSAGE(STATUS "Using Intel C/C++ Compiler")
    set(ICC)
endif (CMAKE_C_COMPILER MATCHES "icc")

set(CMAKE_CXX_STANDARD 11)

if (ICC)
    set(CMAKE_CXX_FLAGS -O3\ -xHOST\ -ipo)
    set(CMAKE_C_FLAGS -O3\ -xHOST\ -ipo)
else (ICC)
    set(CMAKE_CXX_FLAGS -O3)
    set(CMAKE_C_FLAGS -O3)
endif(ICC)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(sincos    math.h   HAVE_SINCOS)
check_cxx_symbol_exists(__sincos  math.h   HAVE___SINCOS)

if(HAVE_SINCOS)
    add_definitions(-DHAVE_SINCOS)
endif()
if(HAVE___SINCOS)
    add_definitions(-DHAVE___SINCOS)
endif()

add_executable(parallel src/parallel.h src/parallel.cpp src/args.cpp src/args.h src/error.cpp src/error.h src/strings.cpp src/strings.h src/macros.h src/gcc_version.h app/main.cpp)