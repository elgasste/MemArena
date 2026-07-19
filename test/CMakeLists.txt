cmake_minimum_required(VERSION 3.15)

add_executable(run_mem_arena_tests
	../src/mem_arena.c
	vendor/unity.c
	test_mem_arena.c
)

set_property(TARGET run_mem_arena_tests PROPERTY COMPILE_WARNING_AS_ERROR ON)
if(MSVC)
    target_compile_options(run_mem_arena_tests PRIVATE /W4)
endif()

target_include_directories(run_mem_arena_tests PRIVATE vendor)
target_include_directories(run_mem_arena_tests PRIVATE ../include)
target_include_directories(run_mem_arena_tests PRIVATE ../src)

add_test( NAME Unity_MemArena_Tests COMMAND run_mem_arena_tests)
