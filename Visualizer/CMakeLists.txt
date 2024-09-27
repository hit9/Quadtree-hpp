cmake_minimum_required(VERSION 3.10)

project(QuadtreeVisualizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

file(GLOB SOURCES *.cpp)
add_executable(QuadtreeVisualizer ${SOURCES})

find_package(SDL2)
find_package(spdlog)
find_package(argparse)

include_directories("../Source")

target_link_libraries(QuadtreeVisualizer SDL2::SDL2main spdlog::spdlog
                      argparse::argparse)
