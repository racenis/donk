cmake_minimum_required(VERSION 3.15)
project(donk C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(donk
	src/array.c
	src/coding.c
	src/container.c
	src/frame.c
	src/memory.c
	src/stream.c
	src/utils.c
)
target_include_directories(donk PUBLIC src)

include(FetchContent)
FetchContent_Declare(
	utest
	URL https://raw.githubusercontent.com/sheredom/utest.h/master/utest.h
	DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(utest)

enable_testing()

add_executable(test_memory tests/memory.c)
target_include_directories(test_memory PRIVATE src ${utest_SOURCE_DIR})
target_link_libraries(test_memory PRIVATE donk)
add_test(NAME memory COMMAND test_memory)