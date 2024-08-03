cmake_minimum_required(VERSION 3.10)

project(quadtree-visualizer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

file(GLOB SOURCES *.cpp)
add_executable(quadtree-visualizer ${SOURCES})

find_package(SDL2)
find_package(spdlog)
find_package(argparse)

include_directories("..")

target_link_libraries(quadtree-visualizer SDL2::SDL2main spdlog::spdlog
    argparse::argparse)
