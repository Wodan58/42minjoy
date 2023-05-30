#
#   module  : CMakeLists.txt
#   version : 1.3
#   date    : 05/30/23
#
cmake_minimum_required(VERSION 3.0)
project(42minjoy VERSION 1.0)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
set(CMAKE_BUILD_TYPE Debug)
endif()
if(WIN32)
set(CMAKE_BUILD_TYPE Release)
option(RUN_TESTS "Run standard tests" OFF)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
option(RUN_TESTS "Run standard tests" OFF)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -g -O0 --coverage -fprofile-arcs -ftest-coverage") # debug, no optimisation
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage") # enabling coverage
endif()
endif()
add_executable(joy joy.c)
add_custom_target(demo01.txt ALL
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS joy
                  COMMAND joy <tutorial.joy >demo01.txt)
add_custom_target(demo02.txt ALL
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  DEPENDS joy
                  COMMAND joy fib.lib fib.joy >demo02.txt)
if(RUN_TESTS)
add_subdirectory(test)
endif()
