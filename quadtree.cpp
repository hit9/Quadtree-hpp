#include "quadtree.hpp"

#include <algorithm>  // for std::max
#include <iostream>   // TODO: remove

namespace quadtree {

static const std::size_t __FNV_BASE = 14695981039346656037ULL;
static const std::size_t __FNV_PRIME = 1099511628211ULL;

NodeId pack(uint64_t d, uint64_t x, uint64_t y, uint64_t w, uint64_t h) {
  // The following magic numbers:
  // 0xfc00000000000000 : the highest 6 bits are all 1, the other bits are all 0.
  // 0x3ffffffe0000000  : the highest 6 bits are all 0, the following 29 bits are all 1,
  // and the
  //                      other bits are all 0.
  // 0x1fffffff         : the lowest 29 bits are all 1, the other bits are all 0.
  return ((d << 58) & 0xfc00000000000000ULL) |
         ((((1 << d) * x / h) << 29) & 0x3ffffffe0000000ULL) |
         (((1 << d) * y / w) & 0x1fffffffULL);
}

bool ObjectKey::operator==(const ObjectKey& other) const {
  return x == other.x && y == other.y && o == other.o;
}

std::size_t ObjectKeyHasher::operator()(const ObjectKey& k) const {
  // pack x and y into a single uint64_t integer.
  uint64_t a = ((k.x << 29) & 0x3ffffffe0000000) | (k.y & 0x1fffffff);
  // hash the object pointer to a size_t integer.
  std::size_t b = std::hash<decltype(k.o)>{}(k.o);
  // combine them via FNV hash.
  std::size_t h = __FNV_BASE;
  h ^= a;
  h *= __FNV_PRIME;
  h ^= b;
  h *= __FNV_PRIME;
  return h;
}

Node::Node(bool isLeaf, int d, int x1, int y1, int x2, int y2)
    : isLeaf(isLeaf), d(d), x1(x1), y1(y1), x2(x2), y2(y2) {
  memset(children, 0, sizeof children);
}

Node::~Node() {
  for (int i = 0; i < 4; i++) {
    if (children[i] != nullptr) {
      delete children[i];
      children[i] = nullptr;
    }
  }
}

__Quadtree::__Quadtree(int w, int h, SplitingStopper ssf) : w(w), h(h), ssf(ssf) {
  memset(numDepthTable, 0, sizeof numDepthTable);
  root = createNode(true, 0, 0, 0, h - 1, w - 1);
  trySplitDown(root);
}

__Quadtree::~__Quadtree() {
  m.clear();
  delete root;
  root = nullptr;
  memset(numDepthTable, 0, sizeof numDepthTable);
  w = 0, h = 0, maxd = 0, numLeafNodes = 0, numObjects = 0;
}

// Using binary search to guess the depth of the target node.
// Reason: the id = (d, x*2^d/h, y*2^d/w), it's the same for all (x,y) inside the same
// node. If id(d,x,y) is not found in the map m, the guessed depth is too large, we should
// shrink the upper bound. Otherwise, if we found a node, but it's not a leaf, the answer
// is too small, we should shrink the lower bound, And finally, if we found a leaf node,
// it's the correct answer. The time complexity is O(log maxd), where maxd is the depth of
// this whole tree.
std::pair<NodeId, Node*> __Quadtree::find(int x, int y) {
  int l = 0, r = maxd - 1;
  while (l <= r) {
    int d = (l + r) >> 1;
    auto id = pack(d, x, y, w, h);
    auto it = m.find(id);
    if (it == m.end()) {  // too large
      r = d - 1;
    } else {
      auto node = it->second;
      if (node->isLeaf) return {id, node};
      l = d + 1;  // too small
    }
  }
  return {0, nullptr};
}

// Add an object located at position (x,y) to the right leaf node.
// And split down if the node is able to continue the spliting after the insertion.
// Does nothing if the given position is out of boundary.
void __Quadtree::add(int x, int y, void* o) {
  // boundary checks.
  if (!(x >= 0 && x < h && y >= 0 && y < w)) return;
  // find the leaf node.
  auto node = find(x, y).second;
  if (node == nullptr) return;
  // add the object to this leaf node.
  auto [_, inserted] = node->objects.insert({x, y, o});
  if (inserted) {
    ++numObjects;
    // try to split down if possible.
    trySplitDown(node);
  }
}

// Returns the parent of given non-root node.
// The node passed in here should guarantee not be root.
Node* __Quadtree::parentOf(Node* node) { return m[pack(node->d - 1, node->x1, node->y1, w, h)]; }

// Indicates whether given rectangle with n number of objects inside it is splitable.
// Return true meaning that this rectangle should be managed by a leaf node.
// Otherwise returns false meaning it should be managed by a non-leaf node.
bool __Quadtree::splitable(int x1, int y1, int x2, int y2, int n) {
  // We can't split if it's a single cell.
  if (x1 == x2 && y1 == y2) return false;
  // We can't split if there's a ssf function stops it.
  if (ssf != nullptr && ssf(y2 - y1 + 1, x2 - x1 + 1, n)) return false;
  return true;
}

// createNode is a simple function to create a new node and add to the global node table.
Node* __Quadtree::createNode(bool isLeaf, int d, int x1, int y1, int x2, int y2) {
  auto node = new Node(isLeaf, d, x1, y1, x2, y2);
  m.insert({pack(d, x1, y1, w, h), node});
  if (isLeaf) ++numLeafNodes;
  // maintains the max depth.
  maxd = std::max(maxd, d);
  ++numDepthTable[d];
  return node;
}

// removeLeafNode is a simple function to delete a leaf node and maintain the tree informations.
// Does nothing if the given node is not a leaf node.
void __Quadtree::removeLeafNode(Node* node) {
  if (!node->isLeaf) return;
  // Remove from the global table.
  m.erase(pack(node->d, node->x1, node->y1, w, h));
  // maintains the max depth.
  --numDepthTable[node->d];
  if (node->d == maxd) {
    // If we are removing a leaf node at the largest depth
    // We need to maintain the maxd, it may change.
    while (numDepthTable[maxd] == 0) --maxd;
  }
  // Finally delete this node.
  delete node;
  --numLeafNodes;
}

// try to split given leaf node if possible.
void __Quadtree::trySplitDown(Node* node) {
  if (node->isLeaf && splitable(node->x1, node->y1, node->x2, node->y2, node->objects.size())) {
    splitHelper2(node);
  }
}

// splitHelper1 helps to create nodes recursively until all descendant nodes are not able to split.
// The d is the depth of the node to create.
// The (x1,y1) and (x2, y2) is the upper-left and lower-right corners of the node to create.
// The upstreamObjects is from the upstream node, we should filter the ones inside the
// rectangle (x1,y1,x2,y2) if this node is going to be a leaf node.
Node* __Quadtree::splitHelper1(int d, int x1, int y1, int x2, int y2, Objects& upstreamObjects) {
  // boundary checks.
  if (!(x1 >= 0 && x1 < h && y1 >= 0 && y1 < w)) return nullptr;
  if (!(x2 >= 0 && x2 < h && y2 >= 0 && y2 < w)) return nullptr;
  if (!(x1 <= x2 && y1 <= y2)) return nullptr;
  // steal objects inside this rectangle from upstream.
  Objects objs;
  for (const auto& k : upstreamObjects) {
    auto [x, y, o] = k;
    if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
      objs.insert(k);
      upstreamObjects.erase(k);
    }
  }
  // Creates a leaf node if the rectangle is not able to split any more.
  if (!splitable(x1, y1, x2, y2, objs.size())) {
    std::cout << x1 << "," << y1 << "," << x2 << "," << y2 << std::endl;
    auto node = createNode(true, d, x1, y1, x2, y2);
    node->objects.swap(objs);
    return node;
  }
  // Creates a non-leaf node if the rectangle is able to split,
  // and then continue to split down recursively.
  auto node = createNode(false, d, x1, y1, x2, y2);
  splitHelper2(node);
  return node;
}

// splitHelper2 helps to split given node into 4 children.
// The node passing in should guarantee:
// 1. it's a leaf node (with empty children).
// 2. or it's marked to be a non-leaf node (with empty children).
// After this call, the node always turns into a non-leaf node.
void __Quadtree::splitHelper2(Node* node) {
  auto x1 = node->x1, y1 = node->y1;
  auto x2 = node->x2, y2 = node->y2;
  auto d = node->d;
  // the following (x3,y3) is the middle point*:
  //
  //     y1    y3       y2
  //  x1 -+------+------+-
  //      |  0   |  1   |
  //  x3  |    * |      |
  //     -+------+------+-
  //      |  2   |  3   |
  //      |      |      |
  //  x2 -+------+------+-
  int x3 = x1 + (x2 - x1) / 2, y3 = y1 + (y2 - y1) / 2;
  node->children[0] = splitHelper1(d + 1, x1, y1, x3, y3, node->objects);
  node->children[1] = splitHelper1(d + 1, x1, y3 + 1, x3, y2, node->objects);
  node->children[2] = splitHelper1(d + 1, x3 + 1, y1, x2, y3, node->objects);
  node->children[3] = splitHelper1(d + 1, x3 + 1, y3 + 1, x2, y2, node->objects);

  // anyway, it's not a leaf node any more.
  if (node->isLeaf) {
    node->isLeaf = false;
    --numLeafNodes;
  }
}

// Remove an object o at position (x,y) from its leaf node.
// And then merge up if this node's parent turns from splitable to not-splitable
// after the removal.
// Does nothing if the given position is out of boundary.
void __Quadtree::remove(int x, int y, void* o) {
  // boundary checks.
  if (!(x >= 0 && x < h && y >= 0 && y < w)) return;
  // find the leaf node.
  auto node = find(x, y).second;
  if (node == nullptr) return;
  // remove the object from this node.
  if (node->objects.erase({x, y, o}) > 0) {
    --numObjects;
    // try to merge up.
    tryMergeUp(node);
  }
}

// A non-leaf node should stay in a not-splitable state, if given leaf node's parent turns to
// not-splitable, we merge the node with its brothers into this parent.
// Does nothing if this node itself is the root.
// Does nothing if this node itself is not a leaf node.
void __Quadtree::tryMergeUp(Node* node) {
  // The root stops to merge up.
  if (node == root) return;
  // We can only merge from the leaf node.
  if (!node->isLeaf) return;
  // The parent of this leaf node.
  auto parent = parentOf(node);
  // Count the objects inside the parent.
  int n = 0;
  for (int i = 0; i < 4; i++) n += parent->children[i]->objects.size();
  // Check if the parent node should be a leaf node now.
  // If it's still splitable, then it should stay be a non-leaf node.
  if (splitable(parent->x1, parent->y1, parent->x2, parent->y2, n)) return;
  // Merges the managed objects up into the parent's objects.
  for (int i = 0; i < 4; i++) {
    for (const auto& k : parent->children[i]->objects) {
      parent->objects.insert(k);
    }
    removeLeafNode(parent->children[i]);
    parent->children[i] = nullptr;
  }
  // It's now turns to be a leaf node.
  parent->isLeaf = true;
  ++numLeafNodes;
  // Continue the merging to the parent, until the root or some parent is splitable.
  tryMergeUp(parent);
}

}  // namespace quadtree
