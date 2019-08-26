42minjoy
--------

Build|Linux|Windows|Coverity|Coverage|Codecov|Quality|Goto
---|---|---|---|---|---|---|---
status|[![Travis CI build status](https://travis-ci.org/Wodan58/42minjoy.svg?branch=master)](https://travis-ci.org/Wodan58/42minjoy)|[![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/github/Wodan58/42minjoy?branch=master&svg=true)](https://ci.appveyor.com/project/Wodan58/42minjoy)|[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/14611.svg)](https://scan.coverity.com/projects/wodan58-42minjoy)|[![Coverage Status](https://coveralls.io/repos/github/Wodan58/42minjoy/badge.svg?branch=master)](https://coveralls.io/github/Wodan58/42minjoy?branch=master)|[![Codecov](https://codecov.io/gh/Wodan58/42minjoy/branch/master/graph/badge.svg)](https://codecov.io/gh/Wodan58/42minjoy)|[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/Wodan58/42minjoy.svg?logo=lgtm&logoWith=18)](https://lgtm.com/projects/g/Wodan58/42minjoy/context:cpp)|[![goto counter](https://img.shields.io/github/search/Wodan58/42minjoy/goto.svg)](https://github.com/Wodan58/42minjoy/search?q=goto)

Joy from [Sympas](https://github.com/nickelsworth/sympas/blob/master/text/18-minijoy.org) translated to C.

Changes
-------

Adding a new builtin, e.g. getch or putch, requires modification of the program
in 4 locations: standardident{}, initialise(), standardident_NAMES[], and joy().
After translating from Pascal to C with the help of
[p2c](https://github.com/FranklinChen/p2c) some corrections were done, marked with R.W.
Reading the library files twice is not needed, given the nature of the language
and that is why some code has been added, removing that feature.

Installation
------------

    cmake .
    cmake --build .

Debugging
---------

Although this software contains no bugs, steps have been taken that make debugging easier.
For a start, gdb requires that input comes from a file, so a typical session will be:

    gdb joy
    ...
    run 42minjoy.lib tutorial.joy
    ...
    quit

If the program crashes gdb sometimes answers to the command `bt` with: No stack.
In that case, it might be beneficial that it is also possible to compile with -DDEBUG
and get a trace of program execution in joy.log.
