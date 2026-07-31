# MemArena

A simple memory arena written in C. For more detailed information, check out the [Wiki](https://github.com/elgasste/MemArena/wiki).

# How To Build

This project is intended to use either Visual Studio or VSCode to build and run, and makes use of CMake to do so.

## Visual Studio

This is the easiest option, all you have to do is select "Open a local folder" and select the MemArena folder, everything should be automatically configured from there. You should even be able to run/debug the unit tests by pressing F5.

## VSCode

You'll need to install the C/C++ Extension Pack plug-in for VSCode, as well as some kind of native C compiler, which you'll need to select when running the project for the first time. At that point you should be able to run/debug unit tests by clicking the "Testing" icon and selecting the desired test suite.

# Performance Utility

A random allocation/free performance utility is included as the `mem_arena_perf` executable.

Arguments:

`mem_arena_perf [arenaBytes] [operations] [maxLiveAllocs] [minAlloc] [maxAlloc] [seed]`

Defaults:

- arenaBytes: `16777216` (16 MB)
- operations: `250000`
- maxLiveAllocs: `4096`
- minAlloc: `8`
- maxAlloc: `2048`
- seed: current time

Example:

`mem_arena_perf 33554432 500000 8192 8 4096 12345`

The utility reports allocation success/failure counts, elapsed time, operations/second, peak live bytes, and arena stats just before cleaning up.
