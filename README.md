42minjoy
========

Build|Linux
---|---
status|[![GitHub CI build status](https://github.com/Wodan58/42minjoy/actions/workflows/cmake.yml/badge.svg)](https://github.com/Wodan58/42minjoy/actions/workflows/cmake.yml)

Joy from [Sympas](https://github.com/nickelsworth/sympas/blob/master/text/18-minijoy.org) translated to C.

Changes
-------

Adding a new builtin, e.g. getch or putch, requires modification of the program
in 4 locations: standardident{}, initialise(), standardident\_NAMES[], and
joy().  After translating from Pascal to C with the help of
[p2c](https://github.com/Classic-Tools/p2c) some corrections were done.

Installation
------------

    cd build
    cmake -G "Unix Makefiles" ..
    cmake --build .
