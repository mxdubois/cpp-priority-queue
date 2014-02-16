sportsball
==========

A simple C++ implementation of a priority queue backed by a heap-sorted array.

Dependencies
------------
The project depends on C++11. GCC 4.8+ is recommended.

Build
-----
I've included the makefile Eclipse generated for me. It is in the `Default` folder.
You should be able to build it with:

    make -f makefile

Running it
----------
You'll need to point the program to one of the datafiles in the `data`
directory. For example:

    sportsball ../data/sportsball1.txt [initialSize] [stepSize]

The `initialSize` is the initial size of the backing array. The `stepSize`
is the increment by which the array length is increased when it is filled.
