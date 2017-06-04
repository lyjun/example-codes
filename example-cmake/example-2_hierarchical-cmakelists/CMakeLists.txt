cmake_minimum_required(VERSION 3.8.1)
project (hierarchical-cmakelists)
message("===> start to build.... " ${PROJECT_NAME})

add_subdirectory(src1)
add_subdirectory(src2)

