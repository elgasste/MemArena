cmake_minimum_required(VERSION 3.15)
project(MemArena LANGUAGES C)
set(CMAKE_C_STANDARD 99)

enable_testing()

include_directories(src)
add_subdirectory(test)
