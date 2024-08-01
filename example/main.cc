#include <iostream>  // TODO: remove

#include "quadtree.hpp"

int main(void) {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return w <= 2 && h <= 2; };
  quadtree::Quadtree tree(6, 6, ssf);
  //  tree.Add(1, 1, nullptr);
  //  tree.Add(2, 2, nullptr);
  //  tree.Remove(1, 1, nullptr);
  std::cout << "size of tree: " << tree.NumNodes() << std::endl;
  std::cout << "leaf nodes of tree: " << tree.NumLeafNodes() << std::endl;
  std::cout << "depth of tree: " << tree.Depth() << std::endl;

  // x x | x | x x | x
  // x x | x | x x | x
  // -------------------
  // x x | x | x x | x
  // -------------------
  // x x | x | x x | x
  // x x | x | x x | x
  // --------------------
  // x x | x | x x | x
  //
  // n = 1
  // n += 4
  // n += 16

  return 0;
}
