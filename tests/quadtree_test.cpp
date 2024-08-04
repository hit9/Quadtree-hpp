#include "quadtree.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("simple square 8x8") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (w <= 2 && h <= 2) || n <= 1; };
  quadtree::Quadtree<int> tree(8, 8, ssf);
  REQUIRE(tree.NumNodes() == 0);
  REQUIRE(tree.NumLeafNodes() == 0);
  REQUIRE(tree.Depth() == 0);
  tree.Build();
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 0);
  // Add (2,3)
  tree.Add(2, 3, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.NumObjects() == 1);
  // Add(3,4)
  tree.Add(3, 4, 1);
  REQUIRE(tree.NumNodes() == 5);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 2);
  // Add(1,5)
  tree.Add(1, 5, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 3);
  // Add(0,4)
  tree.Add(0, 4, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 4);
  // Find (5,2)
  auto node1 = tree.Find(5, 2);
  REQUIRE(node1 != nullptr);
  REQUIRE(node1->x1 == 4);
  REQUIRE(node1->y1 == 0);
  REQUIRE(node1->x2 == 7);
  REQUIRE(node1->y2 == 3);
  // Find (0,0)
  auto node2 = tree.Find(0, 0);
  REQUIRE(node2 != nullptr);
  REQUIRE(node2->x1 == 0);
  REQUIRE(node2->y1 == 0);
  REQUIRE(node2->x2 == 3);
  REQUIRE(node2->y2 == 3);
  // QueryRange (hit 2)
  quadtree::Objects<int> nodes1;
  decltype(tree)::CollectorT c1 = [&nodes1](int x, int y, int o) { nodes1.insert({x, y, o}); };
  tree.QueryRange(1, 2, 4, 4, c1);
  REQUIRE(nodes1.size() == 2);
  REQUIRE(nodes1.find({2, 3, 1}) != nodes1.end());
  REQUIRE(nodes1.find({3, 4, 1}) != nodes1.end());
  // QueryRange (hit 0)
  quadtree::Objects<int> nodes2;
  decltype(tree)::CollectorT c2 = [&nodes2](int x, int y, int o) { nodes2.insert({x, y, o}); };
  tree.QueryRange(4, 1, 5, 5, c2);
  REQUIRE(nodes2.size() == 0);
  // Remove (0,0) // does nothing
  tree.Remove(0, 0, 1);
  REQUIRE(tree.NumObjects() == 4);
  // Remove (1,5) // not affect splition
  tree.Remove(1, 5, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 3);
  // Remove (3,4), merge
  tree.Remove(3, 4, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 2);
  // Remove (2,3), merge
  tree.Remove(2, 3, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 1);
}

TEST_CASE("simple rectangle 7x6") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (w <= 2 && h <= 2) || n <= 1; };
  quadtree::Quadtree<int> tree(7, 6, ssf);
  REQUIRE(tree.NumNodes() == 0);
  REQUIRE(tree.NumLeafNodes() == 0);
  REQUIRE(tree.Depth() == 0);
  tree.Build();
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 0);
  // Add (4,4)
  tree.Add(4, 4, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 1);
  // Add (3,3)
  tree.Add(3, 3, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 2);
  // Find (0,0)
  auto a = tree.Find(0, 0);
  REQUIRE(a != nullptr);
  REQUIRE(a->x1 == 0);
  REQUIRE(a->y1 == 0);
  REQUIRE(a->x2 == 2);
  REQUIRE(a->y2 == 3);
  REQUIRE(a->d == 1);
  // Find (1,5)
  auto b = tree.Find(1, 5);
  REQUIRE(b != nullptr);
  REQUIRE(b->x1 == 0);
  REQUIRE(b->y1 == 4);
  REQUIRE(b->x2 == 2);
  REQUIRE(b->y2 == 6);
  REQUIRE(b->d == 1);
  // Find (3,3)
  auto c = tree.Find(3, 3);
  REQUIRE(c != nullptr);
  REQUIRE(c->x1 == 3);
  REQUIRE(c->y1 == 0);
  REQUIRE(c->x2 == 5);
  REQUIRE(c->y2 == 3);
  REQUIRE(c->d == 1);
  // Find (4,4)
  auto d = tree.Find(4, 4);
  REQUIRE(d != nullptr);
  REQUIRE(d->x1 == 3);
  REQUIRE(d->y1 == 4);
  REQUIRE(d->x2 == 5);
  REQUIRE(d->y2 == 6);
  REQUIRE(d->d == 1);
  // Add (1,2)
  tree.Add(1, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 3);
  // Add (1,3)
  tree.Add(1, 3, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 3);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 4);
  // Add (0,2)
  tree.Add(0, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 3);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 5);
  // Add (1,5)
  tree.Add(1, 5, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 3);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 6);
  // Add (2,5)
  tree.Add(2, 5, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 4 + 2);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 7);
  // Query [(1,1),(5,4)]
  quadtree::Objects<int> nodes1;
  decltype(tree)::CollectorT c1 = [&nodes1](int x, int y, int o) { nodes1.insert({x, y, o}); };
  tree.QueryRange(1, 1, 5, 4, c1);
  REQUIRE(nodes1.size() == 4);
  REQUIRE(nodes1.find({1, 2, 1}) != nodes1.end());
  REQUIRE(nodes1.find({1, 3, 1}) != nodes1.end());
  REQUIRE(nodes1.find({3, 3, 1}) != nodes1.end());
  REQUIRE(nodes1.find({4, 4, 1}) != nodes1.end());
  // Query [(1,4),(5,4)]
  quadtree::Objects<int> nodes2;
  decltype(tree)::CollectorT c2 = [&nodes2](int x, int y, int o) { nodes2.insert({x, y, o}); };
  tree.QueryRange(1, 4, 5, 4, c2);
  REQUIRE(nodes2.size() == 1);
  REQUIRE(nodes2.find({4, 4, 1}) != nodes2.end());
  // Remove (1,2)
  tree.Remove(1, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 4 + 2);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 6);
  // Remove (0,2)
  tree.Remove(0, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 5);
  // Remove (2,5)
  tree.Remove(2, 5, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 4);
  // Remove (3,3) (4,4) (1,5)
  tree.Remove(3, 3, 1);
  tree.Remove(4, 4, 1);
  tree.Remove(1, 5, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 1);
}

TEST_CASE("simple invert-ssf 10x8") {
  // Stop to split if there's no object inside it or all cells are placed with objects.
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(10, 8, ssf);
  REQUIRE(tree.NumNodes() == 0);
  REQUIRE(tree.NumLeafNodes() == 0);
  REQUIRE(tree.Depth() == 0);
  tree.Build();
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 0);
  // Add (4,2)
  tree.Add(4, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 4);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 1);
  // Add (5,2)
  tree.Add(5, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 4);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 2);
  // Add (4,0), splits 2 more children
  tree.Add(4, 0, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 3 + 2);
  REQUIRE(tree.Depth() == 4);
  REQUIRE(tree.NumObjects() == 3);
  // Add (4,1), merges with (4,0)
  tree.Add(4, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 4);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 4);
  // Add (5,0), (5,1), merge
  tree.Add(5, 0, 1);
  tree.Add(5, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 6);
}

TEST_CASE("simple invert-ssf 7x5") {
  // Stop to split if there's no object inside it or all cells are placed with objects.
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(7, 5, ssf);
  REQUIRE(tree.NumNodes() == 0);
  REQUIRE(tree.NumLeafNodes() == 0);
  REQUIRE(tree.Depth() == 0);
  tree.Build();
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 0);
  // Add (4,2)
  tree.Add(4, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 2);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 1);
  // Remove (4,2)
  tree.Remove(4, 2, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 0);
}

TEST_CASE("simple invert-ssf 5x8") {
  // Stop to split if there's no object inside it or all cells are placed with objects.
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(5, 8, ssf);
  REQUIRE(tree.NumNodes() == 0);
  REQUIRE(tree.NumLeafNodes() == 0);
  REQUIRE(tree.Depth() == 0);
  tree.Build();
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
  REQUIRE(tree.NumObjects() == 0);
  // Add (2,2)
  tree.Add(2, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 2);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 1);
  // Add (0,2),(1,2)(3,2)
  tree.Add(0, 2, 1);
  tree.Add(1, 2, 1);
  tree.Add(3, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 4);
  // Remove (2,2), (1,2) => split
  tree.Remove(1, 2, 1);
  tree.Remove(2, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 2 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 1 + 2 + 1 + 2);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 2);
}

TEST_CASE("hook functions") {
  // cnt is the counter to track leaf nodes.
  int cnt = 0;
  quadtree::Visitor<int> afterLeafCreated = [&cnt](quadtree::NodeId, quadtree::Node<int>* node) {
    cnt++;
  };
  quadtree::Visitor<int> beforeLeafRemoved = [&cnt](quadtree::NodeId, quadtree::Node<int>* node) {
    cnt--;
  };
  // Stop to split if there's no object inside it or all cells are placed with objects.
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(9, 6, ssf, afterLeafCreated, beforeLeafRemoved);
  tree.Build();
  REQUIRE(cnt == 1);
  // Add (2,2)
  tree.Add(2, 2, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  // Add(2,3)
  tree.Add(2, 3, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  // Add(1,3)
  tree.Add(1, 3, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  // Remove(1,3)
  tree.Remove(1, 3, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  // Remove all
  tree.Remove(2, 3, 1);
  tree.Remove(2, 2, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
}
