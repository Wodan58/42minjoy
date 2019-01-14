#
#   Makefile for 42minjoy
#
cmake_minimum_required(VERSION 3.0)
project(42minjoy VERSION 1.0)
set(CMAKE_BUILD_TYPE Debug)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
if(WIN32)
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage") # debug, no optimisation
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage") # enabling coverage
endif()
add_executable(joy joy.c)
add_subdirectory(test)
add_custom_target(demo01.txt ALL
                  DEPENDS joy
                  COMMAND joy <tutorial.joy >demo01.txt)
add_custom_target(demo02.txt ALL
                  DEPENDS joy
                  COMMAND joy fib.lib fib.joy >demo02.txt || (exit 0))
# (not open for writing)
add_custom_target(demo03.txt ALL
		  DEPENDS joy
		  COMMAND rm 42minjoy.lst || (exit 0)
		  COMMAND rmdir 42minjoy.lst || (exit 0)
		  COMMAND mkdir 42minjoy.lst || (exit 0)
		  COMMAND joy <tutorial.joy || (exit 0)
		  COMMAND rmdir 42minjoy.lst || (exit 0))
# (not open for reading)
add_custom_target(demo04.txt ALL
		  DEPENDS joy
		  COMMAND joy nonexist.joy || (exit 0))
# unexpected end of file
add_custom_target(demo05.txt ALL
		  DEPENDS joy
		  COMMAND joy <example.txt || (exit 0))
# right parenthesis expected
# null address being referenced
add_custom_target(demo06.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam01.joy || (exit 0))
# exceeding alternative radix
# illegal start of scan expr
add_custom_target(demo07.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam02.joy || (exit 0))
# too many include files
add_custom_target(demo08.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam03.joy || (exit 0))
# "A" .. "Z" expected
# "=" expected
# illegal start of scan expr
add_custom_target(demo09.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam04.joy || (exit 0))
# internal in readfactor
add_custom_target(demo10.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam05.joy || (exit 0))
# pass 2: identifier expected
add_custom_target(demo11.txt ALL
		  DEPENDS joy
		  COMMAND joy tutorial.joy || (exit 0))
# (not open for reading)
add_custom_target(demo12.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam06.joy || (exit 0))
# unknown directive
add_custom_target(demo13.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam07.joy || (exit 0))
# ok
add_custom_target(demo14.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam08.joy || (exit 0))
# too many include files
add_custom_target(demo15.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam09.joy || (exit 0))
# errorcount
add_custom_target(demo16.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam10.joy || (exit 0))
# pass 2: "==" expected
add_custom_target(demo17.txt ALL
		  DEPENDS joy
		  COMMAND joy exam11.joy || (exit 0))
# (not open for reading)
add_custom_target(demo18.txt ALL
		  DEPENDS joy
		  COMMAND joy 42minjoy.lib nonexist.joy || (exit 0))
# listing
add_custom_target(demo19.txt ALL
		  DEPENDS joy
		  COMMAND joy example.lib example.joy || (exit 0))
# bad order in library
add_custom_target(demo20.txt ALL
		  DEPENDS joy
		  COMMAND joy exam12.lib exam12.joy || (exit 0))
# runtime errors
add_custom_target(demo21.txt ALL
		  DEPENDS joy
		  COMMAND joy <exam13.joy || (exit 0))
