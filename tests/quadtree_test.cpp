#include "quadtree.hpp"

#include <catch2/catch_test_macros.hpp>
#include <unordered_set>
#include <vector>

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
  // Add (3,2)
  tree.Add(3, 2, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.NumObjects() == 1);
  // Add(4,3)
  tree.Add(4, 3, 1);
  REQUIRE(tree.NumNodes() == 5);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 2);
  // Add(5,1)
  tree.Add(5, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 3);
  // Add(4,0)
  tree.Add(4, 0, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 4);
  // Find (2,5)
  auto node1 = tree.Find(2, 5);
  REQUIRE(node1 != nullptr);
  REQUIRE(node1->x1 == 0);
  REQUIRE(node1->y1 == 4);
  REQUIRE(node1->x2 == 3);
  REQUIRE(node1->y2 == 7);
  // Find (0,0)
  auto node2 = tree.Find(0, 0);
  REQUIRE(node2 != nullptr);
  REQUIRE(node2->x1 == 0);
  REQUIRE(node2->y1 == 0);
  REQUIRE(node2->x2 == 3);
  REQUIRE(node2->y2 == 3);
  // Find(10,-1) , out of boundary
  auto node3 = tree.Find(10, -1);
  REQUIRE(node3 == nullptr);
  // QueryRange (hit 2)
  quadtree::Objects<int> nodes1;
  decltype(tree)::CollectorT c1 = [&nodes1](int x, int y, int o) { nodes1.insert({x, y, o}); };
  tree.QueryRange(2, 1, 4, 4, c1);
  REQUIRE(nodes1.size() == 2);
  REQUIRE(nodes1.find({3, 2, 1}) != nodes1.end());
  REQUIRE(nodes1.find({4, 3, 1}) != nodes1.end());
  // QueryRange (hit 0)
  quadtree::Objects<int> nodes2;
  decltype(tree)::CollectorT c2 = [&nodes2](int x, int y, int o) { nodes2.insert({x, y, o}); };
  tree.QueryRange(1, 4, 5, 5, c2);
  REQUIRE(nodes2.size() == 0);
  // Remove (0,0) // does nothing
  tree.Remove(0, 0, 1);
  REQUIRE(tree.NumObjects() == 4);
  // Remove (5,1) // not affect splition
  tree.Remove(5, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 3);
  // Remove (4,3), merge
  tree.Remove(4, 3, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 2);
  // Remove (3,2), merge
  tree.Remove(3, 2, 1);
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
  REQUIRE(a->x2 == 3);
  REQUIRE(a->y2 == 2);
  REQUIRE(a->d == 1);
  // Find (5,1)
  auto b = tree.Find(5, 1);
  REQUIRE(b != nullptr);
  REQUIRE(b->x1 == 4);
  REQUIRE(b->y1 == 0);
  REQUIRE(b->x2 == 6);
  REQUIRE(b->y2 == 2);
  REQUIRE(b->d == 1);
  // Find (3,3)
  auto c = tree.Find(3, 3);
  REQUIRE(c != nullptr);
  REQUIRE(c->x1 == 0);
  REQUIRE(c->y1 == 3);
  REQUIRE(c->x2 == 3);
  REQUIRE(c->y2 == 5);
  REQUIRE(c->d == 1);
  // Find (4,4)
  auto d = tree.Find(4, 4);
  REQUIRE(d != nullptr);
  REQUIRE(d->x1 == 4);
  REQUIRE(d->y1 == 3);
  REQUIRE(d->x2 == 6);
  REQUIRE(d->y2 == 5);
  REQUIRE(d->d == 1);
  // Add (2,1)
  tree.Add(2, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 3);
  // Add (3,1)
  tree.Add(3, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 3);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 4);
  // Add (2,0)
  tree.Add(2, 0, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 3);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 5);
  // Add (5,1)
  tree.Add(5, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 3);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 6);
  // Add (5,2)
  tree.Add(5, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 4 + 2);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 7);
  // Query [(1,1),(4,5)]
  quadtree::Objects<int> nodes1;
  decltype(tree)::CollectorT c1 = [&nodes1](int x, int y, int o) { nodes1.insert({x, y, o}); };
  tree.QueryRange(1, 1, 4, 5, c1);
  REQUIRE(nodes1.size() == 4);
  REQUIRE(nodes1.find({2, 1, 1}) != nodes1.end());
  REQUIRE(nodes1.find({3, 1, 1}) != nodes1.end());
  REQUIRE(nodes1.find({3, 3, 1}) != nodes1.end());
  REQUIRE(nodes1.find({4, 4, 1}) != nodes1.end());
  // Query [(4,1),(4,5)]
  quadtree::Objects<int> nodes2;
  decltype(tree)::CollectorT c2 = [&nodes2](int x, int y, int o) { nodes2.insert({x, y, o}); };
  tree.QueryRange(4, 1, 4, 5, c2);
  REQUIRE(nodes2.size() == 1);
  REQUIRE(nodes2.find({4, 4, 1}) != nodes2.end());
  // Remove (2,1)
  tree.Remove(2, 1, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 4 + 4 + 2);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 6);
  // Remove (2,0)
  tree.Remove(2, 0, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 5);
  // Remove (5,2)
  tree.Remove(5, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4);
  REQUIRE(tree.NumLeafNodes() == 4);
  REQUIRE(tree.Depth() == 1);
  REQUIRE(tree.NumObjects() == 4);
  // Remove (3,3) (4,4) (5,1)
  tree.Remove(3, 3, 1);
  tree.Remove(4, 4, 1);
  tree.Remove(5, 1, 1);
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
  // Add (2,4)
  tree.Add(2, 4, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 4);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 1);
  // Add (2,5)
  tree.Add(2, 5, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 4);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 2);
  // Add (0,4), splits 2 more children
  tree.Add(0, 4, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 3 + 2);
  REQUIRE(tree.Depth() == 4);
  REQUIRE(tree.NumObjects() == 3);
  // Add (1,4), merges with (4,0)
  tree.Add(1, 4, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 4);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 4);
  // Add (0,5), (1,5), merge
  tree.Add(0, 5, 1);
  tree.Add(1, 5, 1);
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
  // Add (2,4)
  tree.Add(2, 4, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 3 + 2);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 1);
  // Remove (2,4)
  tree.Remove(2, 4, 1);
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
  // Add (2,0),(2,1)(2,3)
  tree.Add(2, 0, 1);
  tree.Add(2, 1, 1);
  tree.Add(2, 3, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4);
  REQUIRE(tree.NumLeafNodes() == 3 + 4);
  REQUIRE(tree.Depth() == 2);
  REQUIRE(tree.NumObjects() == 4);
  // Remove (2,2), (2,1) => split
  tree.Remove(2, 1, 1);
  tree.Remove(2, 2, 1);
  REQUIRE(tree.NumNodes() == 1 + 4 + 4 + 2 + 2);
  REQUIRE(tree.NumLeafNodes() == 3 + 1 + 2 + 1 + 2);
  REQUIRE(tree.Depth() == 3);
  REQUIRE(tree.NumObjects() == 2);
}

TEST_CASE("hook functions") {
  // cnt is the counter to track leaf nodes.
  int cnt = 0, createdTimes = 0, removedTimes = 0;
  quadtree::Visitor<int> afterLeafCreated = [&](quadtree::Node<int>* node) {
    cnt++;
    createdTimes++;
  };
  quadtree::Visitor<int> afterLeafRemoved = [&](quadtree::Node<int>* node) {
    cnt--;
    removedTimes++;
  };
  // Stop to split if there's no object inside it or all cells are placed with objects.
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(9, 6, ssf, afterLeafCreated, afterLeafRemoved);
  tree.Build();
  REQUIRE(cnt == 1);
  REQUIRE(createdTimes == 1);
  REQUIRE(removedTimes == 0);
  createdTimes = 0, removedTimes = 0;
  // Add (2,2)
  tree.Add(2, 2, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  REQUIRE(createdTimes == 8);
  REQUIRE(removedTimes == 1);
  createdTimes = 0, removedTimes = 0;
  // Add(3,2)
  tree.Add(3, 2, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  REQUIRE(createdTimes == 2);
  REQUIRE(removedTimes == 1);
  createdTimes = 0, removedTimes = 0;
  // Add(3,1)
  tree.Add(3, 1, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  REQUIRE(createdTimes == 4);
  REQUIRE(removedTimes == 1);
  createdTimes = 0, removedTimes = 0;
  // Remove(3,1)
  tree.Remove(3, 1, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  REQUIRE(createdTimes == 1);
  REQUIRE(removedTimes == 4);
  createdTimes = 0, removedTimes = 0;
  // Remove all
  tree.Remove(3, 2, 1);
  tree.Remove(2, 2, 1);
  REQUIRE(tree.NumLeafNodes() == cnt);
  REQUIRE(createdTimes == 1 + 1);
  REQUIRE(removedTimes == 2 + 8);
  createdTimes = 0, removedTimes = 0;
}

TEST_CASE("large 10wx10w") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(1e5, 1e5, ssf);
  tree.Build();
  tree.Add(0, 0, 1);
  // Find (0,0)
  auto node = tree.Find(0, 0);
  REQUIRE(node->x1 == 0);
  REQUIRE(node->y1 == 0);
  REQUIRE(node->d == tree.Depth());
  // Add (1e5/2,1e5/2)
  tree.Add(1e5 / 2, 1e5 / 2, 0);
  // Add (1e5/2+1,1e5/2+1)
  tree.Add(1e5 / 2 + 1, 1e5 / 2 + 1, 0);
  // QueryRange
  quadtree::Objects<int> nodes1;
  decltype(tree)::CollectorT c1 = [&nodes1](int x, int y, int o) { nodes1.insert({x, y, o}); };
  tree.QueryRange((1e5 / 2) - 1, (1e5 / 2) - 1, (1e5 / 2) + 1, (1e5 / 2) + 1, c1);
  REQUIRE(nodes1.size() == 2);
  // Add (5,3)
  tree.Add(5, 3, 0);
  // Remove all
  tree.Remove(1e5 / 2, 1e5 / 2, 0);
  tree.Remove(1e5 / 2 + 1, 1e5 / 2 + 1, 0);
  tree.Remove(5, 3, 0);
  tree.Remove(0, 0, 1);
  REQUIRE(tree.NumNodes() == 1);
  REQUIRE(tree.NumObjects() == 0);
  REQUIRE(tree.Depth() == 0);
}

TEST_CASE("FindSmallestNodeCoveringRange 12x8") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(12, 8, ssf);
  tree.Build();
  // Add (3,3)
  tree.Add(3, 3, 0);
  // Find (3,2),(5,3)
  auto a = tree.FindSmallestNodeCoveringRange(3, 2, 5, 3);
  REQUIRE(a != nullptr);
  REQUIRE(a->d == 2);
  REQUIRE(a->x1 == 3);
  REQUIRE(a->y1 == 2);
  // Find (4,3),(4,3)
  auto b = tree.FindSmallestNodeCoveringRange(4, 3, 4, 3);
  REQUIRE(b != nullptr);
  REQUIRE(b->d == 4);
  REQUIRE(b->x1 == 4);
  REQUIRE(b->y1 == 3);
  // Find (5,3),(3,2)
  auto a1 = tree.FindSmallestNodeCoveringRange(5, 3, 3, 2);
  REQUIRE(a1 == a);
  // Find (1,1),(6,4)
  auto d = tree.FindSmallestNodeCoveringRange(1, 1, 6, 4);
  REQUIRE(d != nullptr);
  REQUIRE(d->d == 0);  // root
  // Out of boundary
  auto e = tree.FindSmallestNodeCoveringRange(-1, -1, 9, 13);
  REQUIRE(e == nullptr);
  // Out of boundary
  auto f = tree.FindSmallestNodeCoveringRange(144, 144, 144, 144);
  REQUIRE(f == nullptr);
}

TEST_CASE("FindNeighbourLeafNodes 12x6") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return (n == 0) || (w * h == n); };
  quadtree::Quadtree<int> tree(12, 6, ssf);
  tree.Build();
  // Add(5,3)
  tree.Add(5, 3, 1);

  // Find sourth neighbor of (0,0)'s leaf node
  std::unordered_set<quadtree::Node<int>*> st1;
  quadtree::Visitor<int> visitor1 = [&](quadtree::Node<int>* node) { st1.insert(node); };
  auto a = tree.Find(0, 0);
  tree.FindNeighbourLeafNodes(a, 2, visitor1);  // S
  REQUIRE(st1.size() == 3);
  REQUIRE(st1.find(tree.Find(0, 3)) != st1.end());
  REQUIRE(st1.find(tree.Find(3, 3)) != st1.end());
  REQUIRE(st1.find(tree.Find(5, 3)) != st1.end());

  // Find north neighbor of (3,3)'s leaf node
  std::unordered_set<quadtree::Node<int>*> st2;
  quadtree::Visitor<int> visitor2 = [&](quadtree::Node<int>* node) { st2.insert(node); };
  auto b = tree.Find(3, 3);
  tree.FindNeighbourLeafNodes(b, 0, visitor2);  // N
  REQUIRE(st2.size() == 1);
  REQUIRE(st2.find(tree.Find(0, 0)) != st2.end());

  // Find west neighbor of (3,6)'s leaf node.
  std::unordered_set<quadtree::Node<int>*> st3;
  quadtree::Visitor<int> visitor3 = [&](quadtree::Node<int>* node) { st3.insert(node); };
  auto c = tree.Find(6, 3);
  tree.FindNeighbourLeafNodes(c, 3, visitor3);  // W
  REQUIRE(st3.size() == 3);
  REQUIRE(st3.find(tree.Find(5, 3)) != st3.end());
  REQUIRE(st3.find(tree.Find(5, 4)) != st3.end());
  REQUIRE(st3.find(tree.Find(5, 5)) != st3.end());

  // Find NE (3,0)'s leaf node.
  std::unordered_set<quadtree::Node<int>*> st4;
  quadtree::Visitor<int> visitor4 = [&](quadtree::Node<int>* node) { st4.insert(node); };
  auto d = tree.Find(0, 3);
  tree.FindNeighbourLeafNodes(d, 5, visitor4);  // NE
  REQUIRE(st4.size() == 1);
  REQUIRE(st4.find(tree.Find(0, 0)) != st4.end());

  // out of region
  // Find NW of (0,0)
  std::unordered_set<quadtree::Node<int>*> st5;
  quadtree::Visitor<int> visitor5 = [&](quadtree::Node<int>* node) { st5.insert(node); };
  auto e = tree.Find(0, 0);
  tree.FindNeighbourLeafNodes(e, 4, visitor5);  // NW
  REQUIRE(st5.size() == 0);

  // out of region
  // Find N of (7,0)
  std::unordered_set<quadtree::Node<int>*> st6;
  quadtree::Visitor<int> visitor6 = [&](quadtree::Node<int>* node) { st6.insert(node); };
  auto f = tree.Find(7, 0);
  tree.FindNeighbourLeafNodes(f, 0, visitor6);  // E
  REQUIRE(st6.size() == 0);

  // Find small node's neighbors
  std::unordered_set<quadtree::Node<int>*> st7;
  quadtree::Visitor<int> visitor7 = [&](quadtree::Node<int>* node) { st7.insert(node); };
  auto g = tree.Find(3, 4);
  tree.FindNeighbourLeafNodes(g, 3, visitor7);  // W
  REQUIRE(st7.size() == 1);
  REQUIRE(st7.find(tree.Find(0, 3)) != st7.end());
}

TEST_CASE("bugfix 50x40 split id correction") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return n == 0 || (w * h == n); };
  quadtree::Quadtree<int> tree(50, 40, ssf);
  tree.Build();
  REQUIRE(tree.NumLeafNodes() == 1);
  tree.Add(5, 3, 1);
  REQUIRE(tree.NumLeafNodes() == 17);
  REQUIRE(tree.Depth() == 6);
  tree.Remove(5, 3, 1);
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.Depth() == 0);
}

TEST_CASE("RemoveObjects") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return n == 0 || (w * h == n); };
  quadtree::Quadtree<int> tree(30, 30, ssf);
  tree.Build();
  REQUIRE(tree.NumLeafNodes() == 1);
  REQUIRE(tree.NumObjects() == 0);

  tree.Add(3, 3, 1);
  tree.Add(3, 3, 2);
  REQUIRE(tree.NumObjects() == 2);
  REQUIRE(tree.NumLeafNodes() > 1);

  tree.RemoveObjects(3, 3);
  REQUIRE(tree.NumObjects() == 0);
  REQUIRE(tree.NumLeafNodes() == 1);
}

TEST_CASE("BatchAddToLeafNode") {
  quadtree::SplitingStopper ssf = [](int w, int h, int n) { return n == 0 || (w * h == n); };
  quadtree::Quadtree<int> tree(50, 40, ssf);
  tree.Build();

  auto root = tree.GetRootNode();
  std::vector<quadtree::BatchOperationItem<int>> items;
  items.push_back({4, 4, 1});
  items.push_back({6, 9, 2});
  items.push_back({7, 10, 3});
  tree.BatchAddToLeafNode(root, items);
  REQUIRE(tree.NumObjects() == 3);
  REQUIRE(tree.NumLeafNodes() == 33);
}
