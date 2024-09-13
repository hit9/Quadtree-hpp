cmake_minimum_required(VERSION 3.10)

project(Quadtree)
set(CMAKE_CXX_STANDARD 20)

add_library(Quadtree INTERFACE)
set_target_properties(Quadtree PROPERTIES PUBLIC_HEADER "Quadtree.hpp")

install(
  TARGETS Quadtree
  LIBRARY DESTINATION ${CMAK_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
