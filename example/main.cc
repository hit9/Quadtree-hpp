#include <iostream>  // TODO: remove

#include "quadtree.hpp"

int main(void) {
  quadtree::SaturationTester st = [](int w, int h, int n) { return w > 10 && h > 10; };
  quadtree::Quadtree<void> tree(100, 50, st);
  //  tree.Add(1, 1, nullptr);
  //  tree.Add(2, 2, nullptr);
  //  tree.Remove(1, 1, nullptr);
  std::cout << "size of tree: " << tree.NumNodes() << std::endl;
  std::cout << "leaf nodes of tree: " << tree.NumLeafNodes() << std::endl;

  return 0;
}
