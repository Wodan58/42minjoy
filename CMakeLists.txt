#
#   Makefile for 42minjoy
#
cmake_minimum_required(VERSION 3.0)
project(42minjoy VERSION 1.0)
set(CMAKE_BUILD_TYPE Debug)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
if(WIN32)
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")     # debug, no optimisation
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage") # enabling coverage
endif()
add_executable(joy joy.c)
#if(WIN32)
#else()
add_custom_target(demo.txt ALL
                  COMMAND joy <tutorial.joy >demo.txt)
#endif()
