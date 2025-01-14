#
#   module  : CMakeLists.txt
#   version : 1.12
#   date    : 01/14/25
#
cmake_minimum_required(VERSION 3.29)
project(42minjoy VERSION 1.0)
option(RUN_TESTS "Run standard tests" OFF)
add_definitions("-g -Wall -Wextra -Wpedantic -Werror -Wno-old-style-definition -Wno-knr-promoted-parameter")
add_executable(joy joy.c gc.c malloc.c free.c setraw.c)
target_link_libraries(joy)
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
