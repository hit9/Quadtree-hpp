// Optimized quadtrees on grid rectangles in C++.
// https://github.com/hit9/quadtree-cpp

#ifndef HIT9_QUADTREE_H
#define HIT9_QUADTREE_H

#include <cstdint>        // for std::uint64_t
#include <cstring>        // for memset
#include <functional>     // for std::function, std::hash
#include <unordered_map>  // for std::unordered_map
#include <unordered_set>  // for std::unordered_set
#include <utility>        // for std::pair

namespace quadtree {

// The maximum width and height of the entire rectangular region.
const int MAX_SIDE = (1 << 29) - 1;
// The maximum depth of a quadtree.
const int MAX_DEPTH = 63;

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
NodeId pack(uint64_t d, uint64_t x, uint64_t y, uint64_t w, uint64_t h);

// Key of node's objects map.
struct ObjectKey {
  int x, y;
  void* o;
  // ObjectKey is comparable.
  bool operator==(const ObjectKey& other) const;
};

// Hasher for ObjectKey.
struct ObjectKeyHasher {
  std::size_t operator()(const ObjectKey& k) const;
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

// Objects is the container to store object pointers and their positions.
using Objects = std::unordered_set<ObjectKey, ObjectKeyHasher>;

// The structure of a tree node.
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
  Objects objects;

  Node(bool isLeaf, int d, int x1, int y1, int x2, int y2);
  ~Node();
};

// Internal quadtree on void* typed object pointer.
class __Quadtree {
 protected:
  using collector_t = std::function<void(void*)>;
  using visitor_t = std::function<void(Node*)>;
  __Quadtree(int w, int h, SplitingStopper ssf = nullptr);
  ~__Quadtree();
  std::pair<NodeId, Node*> find(int x, int y);
  void build(std::function<void*(int x, int y)> get);  // TODO: ref?
  void add(int x, int y, void* o);
  void remove(int x, int y, void* o);
  void queryNode(NodeId id, collector_t& collector);
  void queryRange(int x1, int y1, int x2, int y2, collector_t& collector);
  void foreachLeafNode(visitor_t visitor);
  void foreachNode(visitor_t visitor);
  inline int getNumNodes() { return m.size(); }
  inline int getNumObjects() { return numObjects; }
  inline int getNumLeafNodes() { return numLeafNodes; }
  inline int getDepth() { return maxd; }

 private:
  Node* root = nullptr;
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
  std::unordered_map<NodeId, Node*> m;
  // ~~~~~~~~~~~ Internals ~~~~~~~~~~~~
  Node* parentOf(Node* node);
  bool splitable(int x1, int y1, int x2, int y2, int n1);
  Node* createNode(bool isLeaf, int d, int x1, int y1, int x2, int y2);
  void removeLeafNode(Node* node);
  void trySplitDown(Node* node);
  void tryMergeUp(Node* node);
  Node* splitHelper1(int d, int x1, int y1, int x2, int y2, Objects& upstreamObjects);
  void splitHelper2(Node* node);
};

// Quadtree on a rectangle with width w and height h, storing the pointers to Object.
template <typename Object = void>
class Quadtree : public __Quadtree {
 public:
  // Collector is the function that can collect pointers to the managed objects.
  using Collector = std::function<void(Object*)>;

  Quadtree(int w, int h, SplitingStopper ssf = nullptr) : __Quadtree(w, h, ssf) {}
  ~Quadtree() { __Quadtree::~__Quadtree(); }

  // Returns the total number of objects managed by this tree.
  int NumObjects() { return getNumObjects(); }
  // Returns the number of nodes in this tree.
  int NumNodes() { return getNumNodes(); }
  // Returns the number of leaf nodes in this tree.
  int NumLeafNodes() { return getNumLeafNodes(); }
  // Returns the depth of the tree.
  int Depth() { return getDepth(); }
  // Find the leaf node managing given position (x,y), returns the id of the found node.
  // If the given position crosses the bound, the behavior is undefined.
  // We use binary-search for optimization, the time complexity is O(log Depth).
  NodeId Find(int x, int y) { return find(x, y).first; }
  // Add an object o located at position (x,y) to the tree, and then try to split down
  // the corresponding leaf node, if possible.
  // Does nothing if the given position crosses the boundary.
  void Add(int x, int y, Object* o) { add(x, y, static_cast<void*>(o)); }
  // Remove the managed object located at position (x,y), and then try to merge the
  // corresponding leaf node with its brothers, if possible.
  // Does nothing if the given position crosses the boundary.
  void Remove(int x, int y, Object* o) { remove(x, y, static_cast<void*>(o)); }
  // Query the managed objects in given node, the given collector will be called for each
  // object hits. If the given node is not a leaf, the query will goes recursively down
  // to all its children, and collect their objects all.
  void QueryNode(NodeId id, Collector& collector) {
    collector_t c = [&collector](void* o) { collector(static_cast<Object*>(o)); };
    queryNode(id, c);
  }
  void QueryNode(NodeId id, Collector&& collector) { return QueryNode(id, collector); }
  // Query the objects inside given rectangular range, the given collector will be called
  // for each object hits. The parameters (x1,y1) and (x2,y2) are the upper-left and
  // lower-right corners of the given rectangle.
  void QueryRange(int x1, int y1, int x2, int y2, Collector& collector) {
    collector_t c = [&collector](void* o) { collector(static_cast<Object*>(o)); };
    queryRange(x1, y1, x2, y2, c);
  }
  void QueryRange(int x1, int y1, int x2, int y2, Collector&& collector) {
    QueryRange(x1, y1, x2, y2, collector);
  }
  // Build all nodes recursively on an empty quadtree.
  // The function call get(x,y) should return the pointer to the object located at
  // position (x,y). If there's no object at the requested position, returns nullptr
  // instead. This build function must be called on an **empty** quadtree, where the word "empty"
  // means that there's no objects inside this tree.
  void Build(std::function<Object*(int x, int y)> get) {
    build([&get](int x, int y) -> void* { return static_cast<void*>(get(x, y)); });
  }
};

}  // namespace quadtree
#endif
