#
#   module  : CMakeLists.txt
#   version : 1.10
#   date    : 11/26/24
#
cmake_minimum_required(VERSION 3.29)
project(42minjoy VERSION 1.0)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  option(RUN_TESTS "Run standard tests" ON)
else()
  option(RUN_TESTS "Run standard tests" OFF)
endif()
if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
else()
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -g -O3 --coverage -fprofile-arcs -ftest-coverage") # debug, no optimisation
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage") # enabling coverage
  endif()
endif()
add_executable(joy joy.c)
add_custom_target(demo01.txt ALL
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS joy
                  COMMAND joy tutorial.joy >demo01.txt)
add_custom_target(demo02.txt ALL
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS joy
                  COMMAND joy fib.joy >demo02.txt)
if(RUN_TESTS)
  add_subdirectory(test)
endif()
