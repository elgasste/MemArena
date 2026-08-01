cmake_minimum_required(VERSION 3.15)

add_executable(Test_MemArena
	../src/mem_arena.c
	vendor/unity.c
	test_mem_arena.c
)

add_executable(MemArena_Perf
	../src/mem_arena.c
	mem_arena_perf.c
)

set_property(TARGET Test_MemArena PROPERTY COMPILE_WARNING_AS_ERROR ON)
if(MSVC)
    target_compile_options(Test_MemArena PRIVATE /W4)
    target_compile_options(MemArena_Perf PRIVATE /W4)
endif()

target_include_directories(Test_MemArena PRIVATE vendor)
target_include_directories(Test_MemArena PRIVATE ../include)
target_include_directories(Test_MemArena PRIVATE ../src)

target_include_directories(MemArena_Perf PRIVATE ../include)
target_include_directories(MemArena_Perf PRIVATE ../src)

add_test( NAME Unity_MemArena_Tests COMMAND Test_MemArena)
