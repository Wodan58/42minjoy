#
#   module  : CMakeLists.txt
#   version : 1.14
#   date    : 02/10/25
#
cmake_minimum_required(VERSION 3.29)
project(42minjoy VERSION 1.0)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
else()
add_definitions("-DNCHECK -O3 -Wall -Wextra -Werror -Wno-old-style-definition -Wno-knr-promoted-parameter")
endif()
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
