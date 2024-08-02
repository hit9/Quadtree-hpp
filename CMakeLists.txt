cmake_minimum_required(VERSION 3.10)

project(quadtree)
set(CMAKE_CXX_STANDARD 20)

add_library(quadtree INTERFACE)
set_target_properties(quadtree PROPERTIES PUBLIC_HEADER "quadtree.hpp")

install(
  TARGETS quadtree
  LIBRARY DESTINATION ${CMAK_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
