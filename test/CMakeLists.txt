# (not open for writing)
add_custom_target(demo03.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND rm 42minjoy.lst || (exit 0)
		  COMMAND rmdir 42minjoy.lst || (exit 0)
		  COMMAND mkdir 42minjoy.lst || (exit 0)
		  COMMAND joy <tutorial.joy || (exit 0)
		  COMMAND rmdir 42minjoy.lst || (exit 0))
# (not open for reading)
add_custom_target(demo04.txt ALL
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  DEPENDS joy
		  COMMAND joy nonexist.joy)
# unexpected end of file
add_custom_target(demo05.txt ALL
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  DEPENDS joy
		  COMMAND joy <example.txt)
# right parenthesis expected
# null address being referenced
add_custom_target(demo06.txt ALL
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  DEPENDS joy
		  COMMAND joy <exam01.joy)
# exceeding alternative radix
# illegal start of scan expr
add_custom_target(demo07.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam02.joy)
# too many include files
add_custom_target(demo08.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam03.joy)
# "A" .. "Z" expected
# "=" expected
# illegal start of scan expr
add_custom_target(demo09.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam04.joy)
# internal in readfactor
add_custom_target(demo10.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam05.joy)
# pass 2: identifier expected
add_custom_target(demo11.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy tutorial.joy)
# (not open for reading)
add_custom_target(demo12.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam06.joy)
# unknown directive
add_custom_target(demo13.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam07.joy)
# ok
add_custom_target(demo14.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam08.joy)
# too many include files
add_custom_target(demo15.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam09.joy)
# errorcount
add_custom_target(demo16.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam10.joy)
# pass 2: "==" expected
add_custom_target(demo17.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy exam11.joy)
# (not open for reading)
add_custom_target(demo18.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy 42minjoy.lib nonexist.joy)
# listing
add_custom_target(demo19.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy example.lib example.joy)
# bad order in library
add_custom_target(demo20.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy exam12.lib exam12.joy)
# runtime errors
add_custom_target(demo21.txt ALL
		  DEPENDS joy
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
		  COMMAND joy <exam13.joy)
