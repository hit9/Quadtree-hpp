// Optimized quadtrees on grid rectangles in C++.
// https://github.com/hit9/quadtree-hpp
//
// BSD license. Chao Wang, Version: 0.1.0
//
// Coordinate conventions:
//
//           w
//    +---------------> y
//    |
// h  |
//    |
//    v
//    x
//

#ifndef HIT9_QUADTREE_HPP
#define HIT9_QUADTREE_HPP

#include <cstdint>        // for std::uint64_t
#include <cstring>        // for memset
#include <functional>     // for std::function, std::hash
#include <unordered_map>  // for std::unordered_map
#include <unordered_set>  // for std::unordered_set

namespace quadtree {

// The maximum width and height of the entire rectangular region.
const int MAX_SIDE = (1 << 29) - 1;
// The maximum depth of a quadtree.
const int MAX_DEPTH = 29;

using std::uint64_t;

// NodeId is the unique identifier of a tree node, composed of:
//
// +----- 6bit -----+------- 29bit ----+----- 29bit -----+
// | depth d (6bit) | floor(x*(2^d)/h) | floor(y*(2^d)/w |
// +----------------+------------------+-----------------+
//
// Properties:
// 1. Substituting this formula into any position (x,y) inside the node always give the same ID.
// 2. The id of tree root is always 0.
// 3. The deeper the node, the larger the id.
// 4. For nodes at the same depth, the id changes with the size of x*w+y.
using NodeId = uint64_t;

// pack caculates the id of a node.
// the d is the depth of the node, the (x,y) is any position inside the node's rectangle.
// the w and h is the whole rectangular region managed by the quadtree.
inline NodeId pack(uint64_t d, uint64_t x, uint64_t y, uint64_t w, uint64_t h) {
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

template <typename Object>
struct ObjectKey {
  int x, y;
  Object o;
  // ObjectKey is comparable.
  bool operator==(const ObjectKey& other) const;
};

// Hasher for ObjectKey.
template <typename Object, typename ObjectHasher = std::hash<Object>>
struct ObjectKeyHasher {
  std::size_t operator()(const ObjectKey<Object>& k) const;
};

// SplitingStopper is the type of the function to check if a node should stop to split.
// The parameters here:
//  w (int): the node's rectangle's width.
//  h (int): the node's rectangle's height.
//  n (int): the number of objects managed by the node.
// What's more, if the w and h are both 1, it stops to split anyway.
// Examples:
//  1. to split into small rectangles not too small (e.g. at least 10x10)
//        [](int w, int h, int n) { return w <= 10 && h <= 10; };
//  2. to split into small rectangles contains less than q objects (e.g. q=10)
//        [](int w, int h, int n) { return n < 10; };
using SplitingStopper = std::function<bool(int w, int h, int n)>;

// Objects is the container to store objects and their positions.
template <typename Object, typename ObjectHasher = std::hash<Object>>
using Objects = std::unordered_set<ObjectKey<Object>, ObjectKeyHasher<Object, ObjectHasher>>;

// The structure of a tree node.
template <typename Object, typename ObjectHasher = std::hash<Object>>
struct Node {
  bool isLeaf;
  // d is the depth of this node in the tree, starting from 0.
  // (x1,y1) and (x2,y2) are the upper-left and lower-right corners of the node's rectangle:
  //
  //     (x1,y1) +---------------+
  //             |               |
  //             +---------------+ (x2,y2)
  int d, x1, y1, x2, y2;
  // Children: 0: left-top, 1: right-top, 2: left-bottom, 3: right-bottom
  // For a leaf node, the children of which are all nullptr.
  // For a non-leaf node, there's atleast one non-nullptr child.
  //
  //       +-----+-----+
  //       |  0  |  1  |
  //       +-----+-----+
  //       |  2  |  3  |
  //       +-----+-----+
  Node* children[4];
  // For a leaf node, this container stores the objects managed by this node.
  // For a non-leaf node, this container is empty.
  // The objects container itself is an unordered_set.
  // To iterate this container, just:
  //    for (auto [x, y, o] : objects)
  //       // for each object o locates at position (x,y)
  Objects<Object, ObjectHasher> objects;

  Node(bool isLeaf, int d, int x1, int y1, int x2, int y2);
  ~Node();
};

// Collector is the function that can collect the managed objects.
// The arguments is (x,y,object);
template <typename Object>
using Collector = std::function<void(int, int, Object)>;

// Visitor is the function that can access a quadtree node.
template <typename Object, typename ObjectHasher = std::hash<Object>>
using Visitor = std::function<void(NodeId, Node<Object, ObjectHasher>*)>;

// Quadtree on a rectangle with width w and height h, storing the objects.
// The type parameter Object is the type of the objects to store on this tree.
// Object is required to be comparable (the operator== must be available).
// e.g. Quadtree<int>, Quadtree<Entity*>, Quadtree<void*>
// We store the objects into an unordered_set along with its position,
// the hashing of a object is handled by the second type parameter ObjectHasher.
template <typename Object, typename ObjectHasher = std::hash<Object>>
class Quadtree {
 public:
  using NodeT = Node<Object, ObjectHasher>;
  using CollectorT = Collector<Object>;
  using VisitorT = Visitor<Object, ObjectHasher>;
  using ObjectsT = Objects<Object, ObjectHasher>;

  Quadtree(int w, int h, SplitingStopper ssf = nullptr);
  ~Quadtree();

  // Returns the depth of the tree, starting from 0.
  int Depth() const { return maxd; }
  // Returns the total number of objects managed by this tree.
  int NumObjects() const { return numObjects; }
  // Returns the number of nodes in this tree.
  int NumNodes() const { return m.size(); }
  // Returns the number of leaf nodes in this tree.
  int NumLeafNodes() const { return numLeafNodes; }
  // Build all nodes recursively on an empty quadtree.
  // This build function must be called on an **empty** quadtree,
  // where the word "empty" means that there's no nodes inside this tree.
  void Build();
  // Find the leaf node managing given position (x,y).
  // If the given position crosses the bound, the behavior is undefined.
  // We use binary-search for optimization, the time complexity is O(log Depth).
  NodeT* Find(int x, int y) const;
  // Add an object located at position (x,y) to the right leaf node.
  // And split down if the node is able to continue the spliting after the insertion.
  // Does nothing if the given position is out of boundary.
  // Dose nothing if this object already exist at given position.
  void Add(int x, int y, Object o);
  // Remove the managed object located at position (x,y), and then try to merge the
  // corresponding leaf node with its brothers, if possible.
  // Does nothing if the given position crosses the boundary.
  // Dose nothing if this object dose not exist at given position.
  void Remove(int x, int y, Object o);
  // Query the objects inside given rectangular range, the given collector will be called
  // for each object hits. The parameters (x1,y1) and (x2,y2) are the upper-left and
  // lower-right corners of the given rectangle.
  // Does nothing if x1 <= x2 && y1 <= y2 is not satisfied.
  void QueryRange(int x1, int y1, int x2, int y2, CollectorT& collector) const;
  void QueryRange(int x1, int y1, int x2, int y2, CollectorT&& collector) const;
  // Traverse all nodes in this tree.
  // The order is unstable between two traverses.
  // To traverse only the leaf nodes, you may filter by the `node->isLeaf` attribute.
  void ForEachNode(VisitorT& visitor) const;

 private:
  NodeT* root = nullptr;
  // width and height of the whole region.
  int w, h;
  // maxd is the current maximum depth.
  int maxd = 0;
  // numDepthTable records many nodes reaches every depth.
  int numDepthTable[MAX_DEPTH];
  // the number of objects in this tree.
  int numObjects = 0;
  // the number of leaf nodes in this tree.
  int numLeafNodes = 0;
  // the function to test if a node should stop to split.
  SplitingStopper ssf = nullptr;
  // cache the mappings between id and the node.
  std::unordered_map<NodeId, NodeT*> m;

  // ~~~~~~~~~~~ Internals ~~~~~~~~~~~~~
  NodeT* parentOf(NodeT* node) const;
  bool splitable(int x1, int y1, int x2, int y2, int n) const;
  NodeT* createNode(bool isLeaf, int d, int x1, int y1, int x2, int y2);
  void removeLeafNode(NodeT* node);
  void trySplitDown(NodeT* node);
  void tryMergeUp(NodeT* node);
  NodeT* splitHelper1(int d, int x1, int y1, int x2, int y2, ObjectsT& upstreamObjects);
  void splitHelper2(NodeT* node);
  void queryRange(NodeT* node, CollectorT& collector, int x1, int y1, int x2, int y2) const;
};

// ~~~~~~~~~~~ Implementation ~~~~~~~~~~~~~

template <typename Object>
bool ObjectKey<Object>::operator==(const ObjectKey& other) const {
  return x == other.x && y == other.y && o == other.o;
}

const std::size_t __FNV_BASE = 14695981039346656037ULL;
const std::size_t __FNV_PRIME = 1099511628211ULL;

template <typename Object, typename ObjectHasher>
std::size_t ObjectKeyHasher<Object, ObjectHasher>::operator()(const ObjectKey<Object>& k) const {
  // pack x and y into a single uint64_t integer.
  uint64_t a = ((k.x << 29) & 0x3ffffffe0000000) | (k.y & 0x1fffffff);
  // hash the object to a size_t integer.
  std::size_t b = ObjectHasher{}(k.o);
  // combine them via FNV hash.
  std::size_t h = __FNV_BASE;
  h ^= a;
  h *= __FNV_PRIME;
  h ^= b;
  h *= __FNV_PRIME;
  return h;
}

template <typename Object, typename ObjectHasher>
Node<Object, ObjectHasher>::Node(bool isLeaf, int d, int x1, int y1, int x2, int y2)
    : isLeaf(isLeaf), d(d), x1(x1), y1(y1), x2(x2), y2(y2) {
  memset(children, 0, sizeof children);
}

template <typename Object, typename ObjectHasher>
Node<Object, ObjectHasher>::~Node() {
  for (int i = 0; i < 4; i++) {
    if (children[i] != nullptr) {
      delete children[i];
      children[i] = nullptr;
    }
  }
}

template <typename Object, typename ObjectHasher>
Quadtree<Object, ObjectHasher>::Quadtree(int w, int h, SplitingStopper ssf)
    : w(w), h(h), ssf(ssf) {
  memset(numDepthTable, 0, sizeof numDepthTable);
}

template <typename Object, typename ObjectHasher>
Quadtree<Object, ObjectHasher>::~Quadtree() {
  m.clear();
  delete root;
  root = nullptr;
  memset(numDepthTable, 0, sizeof numDepthTable);
  w = 0, h = 0, maxd = 0, numLeafNodes = 0, numObjects = 0;
}

// Returns the parent of given non-root node.
// The node passed in here should guarantee not be root.
template <typename Object, typename ObjectHasher>
Node<Object, ObjectHasher>* Quadtree<Object, ObjectHasher>::parentOf(NodeT* node) const {
  if (node == nullptr || node == root || node->d == 0) return nullptr;
  return m.at(pack(node->d - 1, node->x1, node->y1, w, h));
}

// Indicates whether given rectangle with n number of objects inside it is splitable.
// Return true meaning that this rectangle should be managed by a leaf node.
// Otherwise returns false meaning it should be managed by a non-leaf node.
template <typename Object, typename ObjectHasher>
bool Quadtree<Object, ObjectHasher>::splitable(int x1, int y1, int x2, int y2, int n) const {
  // We can't split if it's a single cell.
  if (x1 == x2 && y1 == y2) return false;
  // We can't split if there's a ssf function stops it.
  if (ssf != nullptr && ssf(y2 - y1 + 1, x2 - x1 + 1, n)) return false;
  return true;
}

// createNode is a simple function to create a new node and add to the global node table.
template <typename Object, typename ObjectHasher>
Node<Object, ObjectHasher>* Quadtree<Object, ObjectHasher>::createNode(bool isLeaf, int d, int x1,
                                                                       int y1, int x2, int y2) {
  auto node = new NodeT(isLeaf, d, x1, y1, x2, y2);
  m.insert({pack(d, x1, y1, w, h), node});
  if (isLeaf) ++numLeafNodes;
  // maintains the max depth.
  maxd = std::max(maxd, d);
  ++numDepthTable[d];
  return node;
}

// removeLeafNode is a simple function to delete a leaf node and maintain the tree informations.
// Does nothing if the given node is not a leaf node.
template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::removeLeafNode(NodeT* node) {
  if (!node->isLeaf) return;
  // Remove from the global table.
  m.erase(pack(node->d, node->x1, node->y1, w, h));
  // maintains the max depth.
  --numDepthTable[node->d];
  if (node->d == maxd) {
    // If we are removing a leaf node at the largest depth
    // We need to maintain the maxd, it may change.
    // the iterations will certainly smaller than MAX_DEPTH (29);
    while (numDepthTable[maxd] == 0) --maxd;
  }
  // Finally delete this node.
  delete node;
  --numLeafNodes;
}

// splitHelper1 helps to create nodes recursively until all descendant nodes are not able to split.
// The d is the depth of the node to create.
// The (x1,y1) and (x2, y2) is the upper-left and lower-right corners of the node to create.
// The upstreamObjects is from the upstream node, we should filter the ones inside the
// rectangle (x1,y1,x2,y2) if this node is going to be a leaf node.
template <typename Object, typename ObjectHasher>
Node<Object, ObjectHasher>* Quadtree<Object, ObjectHasher>::splitHelper1(
    int d, int x1, int y1, int x2, int y2, ObjectsT& upstreamObjects) {
  // boundary checks.
  if (!(x1 >= 0 && x1 < h && y1 >= 0 && y1 < w)) return nullptr;
  if (!(x2 >= 0 && x2 < h && y2 >= 0 && y2 < w)) return nullptr;
  if (!(x1 <= x2 && y1 <= y2)) return nullptr;
  // steal objects inside this rectangle from upstream.
  ObjectsT objs;
  for (const auto& k : upstreamObjects) {
    auto [x, y, o] = k;
    if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
      objs.insert(k);
    }
  }
  // Erase from upstreamObjects.
  // An object should alwasy go to only one branch.
  for (const auto& k : objs) upstreamObjects.erase(k);
  // Creates a leaf node if the rectangle is not able to split any more.
  if (!splitable(x1, y1, x2, y2, objs.size())) {
    auto node = createNode(true, d, x1, y1, x2, y2);
    node->objects.swap(objs);
    return node;
  }
  // Creates a non-leaf node if the rectangle is able to split,
  // and then continue to split down recursively.
  auto node = createNode(false, d, x1, y1, x2, y2);
  // Add the objects to this node temply, it will finally be stealed
  // by the its descendant leaf nodes.
  node->objects.swap(objs);
  splitHelper2(node);
  return node;
}

// splitHelper2 helps to split given node into 4 children.
// The node passing in should guarantee:
// 1. it's a leaf node (with empty children).
// 2. or it's marked to be a non-leaf node (with empty children).
// After this call, the node always turns into a non-leaf node.
template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::splitHelper2(NodeT* node) {
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

// try to split given leaf node if possible.
template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::trySplitDown(NodeT* node) {
  if (node->isLeaf && splitable(node->x1, node->y1, node->x2, node->y2, node->objects.size())) {
    splitHelper2(node);
  }
}

// A non-leaf node should stay in a not-splitable state, if given leaf node's parent turns to
// not-splitable, we merge the node with its brothers into this parent.
// Does nothing if this node itself is the root.
// Does nothing if this node itself is not a leaf node.
template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::tryMergeUp(NodeT* node) {
  // The root stops to merge up.
  if (node == root) return;
  // We can only merge from the leaf node.
  if (!node->isLeaf) return;

  // The parent of this leaf node.
  auto parent = parentOf(node);
  if (parent == nullptr) return;

  // We can only merge if all the brothers are all leaf nodes.
  for (int i = 0; i < 4; i++) {
    auto child = parent->children[i];
    if (child != nullptr && !child->isLeaf) {
      return;
    }
  }

  // Count the objects inside the parent.
  int n = 0;
  for (int i = 0; i < 4; i++) {
    auto child = parent->children[i];
    if (child != nullptr) {
      n += child->objects.size();
    }
  }

  // Check if the parent node should be a leaf node now.
  // If it's still splitable, then it should stay be a non-leaf node.
  if (splitable(parent->x1, parent->y1, parent->x2, parent->y2, n)) return;

  // Merges the managed objects up into the parent's objects.
  for (int i = 0; i < 4; i++) {
    auto child = parent->children[i];
    if (child != nullptr) {
      for (const auto& k : child->objects) {
        parent->objects.insert(k);  // copy
      }
      removeLeafNode(child);
      parent->children[i] = nullptr;
    }
  }
  // this parent node now turns to be leaf node.
  parent->isLeaf = true;
  ++numLeafNodes;
  // Continue the merging to the parent, until the root or some parent is splitable.
  tryMergeUp(parent);
}

// AABB overlap testing.
// Check if rectangle ((ax1, ay1), (ax2, ay2)) and ((bx1,by1), (bx2, by2)) overlaps.
inline bool isOverlap(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
  // clang-format off
  //  (ax1,ay1)
  //      +--------------+
  //   A  |    (bx1,by1) |
  //      |       +------|-------+
  //      +-------+------+       |   B
  //              |    (ax2,ay2) |
  //              +--------------+ (bx2, by2)
  //
  // Ref: https://silentmatt.com/rectangle-intersection/
  //
  // ax1 < bx2 => A's upper boundary is above B's bottom boundary.
  // ax2 > bx1 => A's bottom boundary is below B's upper boundary.
  //
  //                ***********  B's upper                      A's upper    -----------
  //   A's upper    -----------                       OR                     ***********  B's upper
  //                ***********  B's bottom                     A's bottom   -----------
  //   A's bottom   -----------                                              ***********  B's bottom
  //
  // ay1 < by2 => A's left boundary is on the left of B's right boundary.
  // ay2 > by1 => A's right boundary is on the right of B's left boundary.
  //
  //           A's left         A's right                  A's left        A's right
  //
  //      *       |       *       |              OR           |       *       |        * 
  //      *       |       *       |                           |       *       |        *
  //      *       |       *       |                           |       *       |        *
  //  B's left        B's right                                    B's left          B's right
  //
  // We can also see that, swapping the roles of A and B, the formula remains unchanged.
  //
  // clang-format on
  //
  // And we are processing overlapping on integeral coordinates, the (x1,y1) and (x2,y2) are
  // considered inside the rectangle. so we use <= and >= instead of < and >.
  return ax1 <= bx2 && ax2 >= bx1 && ay1 <= by2 && ay2 >= by1;
}

// Query objects located in given rectangle in the given node.
template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::queryRange(NodeT* node, CollectorT& collector, int x1, int y1,
                                                int x2, int y2) const {
  if (node == nullptr) return;
  // AABB overlap test.
  if (!isOverlap(node->x1, node->y1, node->x2, node->y2, x1, y1, x2, y2)) return;

  if (!node->isLeaf) {
    // recursively down to the children.
    for (int i = 0; i < 4; i++) {
      queryRange(node->children[i], collector, x1, y1, x2, y2);
    }
    return;
  }

  // Collects objects inside the rectangle for this leaf node.
  for (auto [x, y, o] : node->objects)
    if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
      collector(x, y, o);
    }
}

// Using binary search to guess the depth of the target node.
// Reason: the id = (d, x*2^d/h, y*2^d/w), it's the same for all (x,y) inside the same
// node. If id(d,x,y) is not found in the map m, the guessed depth is too large, we should
// shrink the upper bound. Otherwise, if we found a node, but it's not a leaf, the answer
// is too small, we should shrink the lower bound, And finally, if we found a leaf node,
// it's the correct answer. The time complexity is O(log maxd), where maxd is the depth of
// this whole tree.
template <typename Object, typename ObjectHasher>
Node<Object, ObjectHasher>* Quadtree<Object, ObjectHasher>::Find(int x, int y) const {
  int l = 0, r = maxd;
  while (l <= r) {
    int d = (l + r) >> 1;
    auto id = pack(d, x, y, w, h);
    auto it = m.find(id);
    if (it == m.end()) {  // too large
      r = d - 1;
    } else {
      auto node = it->second;
      if (node->isLeaf) return node;
      l = d + 1;  // too small
    }
  }
  return nullptr;
}

template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::Add(int x, int y, Object o) {
  // boundary checks.
  if (!(x >= 0 && x < h && y >= 0 && y < w)) return;
  // find the leaf node.
  auto node = Find(x, y);
  if (node == nullptr) return;
  // add the object to this leaf node.
  auto [_, inserted] = node->objects.insert({x, y, o});
  if (inserted) {
    ++numObjects;
    // try to split down if possible.
    trySplitDown(node);
  }
}

template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::Remove(int x, int y, Object o) {
  // boundary checks.
  if (!(x >= 0 && x < h && y >= 0 && y < w)) return;
  // find the leaf node.
  auto node = Find(x, y);
  if (node == nullptr) return;
  // remove the object from this node.
  if (node->objects.erase({x, y, o}) > 0) {
    --numObjects;
    // try to merge up.
    tryMergeUp(node);
  }
}

template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::Build() {
  root = createNode(true, 0, 0, 0, h - 1, w - 1);
  trySplitDown(root);
}

template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::ForEachNode(VisitorT& visitor) const {
  for (auto [id, node] : m) visitor(id, node);
}

template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::QueryRange(int x1, int y1, int x2, int y2,
                                                CollectorT& collector) const {
  if (!(x1 <= x2 && y1 <= y2)) return;
  queryRange(root, collector, x1, y1, x2, y2);
}

template <typename Object, typename ObjectHasher>
void Quadtree<Object, ObjectHasher>::QueryRange(int x1, int y1, int x2, int y2,
                                                CollectorT&& collector) const {
  QueryRange(x1, y1, x2, y2, collector);
}

}  // namespace quadtree

#endif
