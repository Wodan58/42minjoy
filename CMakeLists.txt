#
#   Makefile for 42minjoy
#
cmake_minimum_required(VERSION 3.0)
project(42minjoy VERSION 1.0)
set(CMAKE_BUILD_TYPE Release)
if(WIN32)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -g -O0 --coverage -fprofile-arcs -ftest-coverage") # debug, no optimisation
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage") # enabling coverage
endif()
add_executable(joy joy.c)
if(WIN32)
else()
add_subdirectory(test)
endif()
add_custom_target(demo01.txt ALL
                  DEPENDS joy
                  COMMAND joy <tutorial.joy >demo01.txt)
add_custom_target(demo02.txt ALL
                  DEPENDS joy
                  COMMAND joy fib.lib fib.joy >demo02.txt)
