42minjoy
--------

Build|Linux|Windows|Coverity
---|---|---|---
status|[![GitHub CI build status](https://github.com/Wodan58/42minjoy/actions/workflows/cmake.yml/badge.svg)](https://github.com/Wodan58/42minjoy/actions/workflows/cmake.yml)|[![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/github/Wodan58/42minjoy?branch=master&svg=true)](https://ci.appveyor.com/project/Wodan58/42minjoy)|[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/14611.svg)](https://scan.coverity.com/projects/wodan58-42minjoy)

Joy from [Sympas](https://github.com/nickelsworth/sympas/blob/master/text/18-minijoy.org) translated to C.

Changes
-------

Adding a new builtin, e.g. getch or putch, requires modification of the program
in 4 locations: standardident{}, initialise(), standardident\_NAMES[], and
joy().  After translating from Pascal to C with the help of
[p2c](https://github.com/Classic-Tools/p2c) some corrections were done.
Reading the library files twice is not needed, given the nature of the language
and that is why this feature was removed. After some more changes it is now
transitioning from a small version of Joy to a full version of Joy.

Installation
------------

    rm -rf build
    mkdir build
    cd build
    CC=gcc CXX=gcc cmake -G "Unix Makefiles" ..
    cmake --build .
    cp joy.exe ..
