cmake_minimum_required(VERSION 3.10)
project(quadtree_example CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories("..")

# Targets
add_executable(quadtree_example main.cc)
