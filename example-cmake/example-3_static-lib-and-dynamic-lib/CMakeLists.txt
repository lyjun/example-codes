cmake_minimum_required(VERSION 3.8.1)
project (static-lib-and-dynamic-lib)
set(CMAKE_BUILD_TYPE Release)

# include headers
include_directories(lib)
# source files
set(SOURCES "lib/foo.cpp")

# share library
add_library(foo-d SHARED ${SOURCES})
# static library
add_library(foo-s STATIC ${SOURCES})

# unit test for libraries
# way 1: add to each 
#add_compile_options(-D_TEST_)
#add_executable(foo-test lib/foo.cpp)
# way 2: only available with this project
add_executable(foo-test lib/foo.cpp)
target_compile_options(
	foo-test 
	PRIVATE -D_TEST_)

# excutable with share library
add_executable(main-d main.cpp)
target_link_libraries(main-d foo-d)
# excutable with static library
add_executable(main-s main.cpp)
target_link_libraries(main-s foo-s)
