quadtree.hpp
============

A quadtree implementation working on grid rectangles in C++.

## Screenshots


| <!-- -->                                           | <!-- -->                                                      |
| -------------------------------------------------- | ------------------------------------------------------------- |
| Sqaure: ![](misc/images/quadtree-square.gif)       | Rectangle *: ![](misc/images/quadtree-rectangle.gif)          |

## Installation

Copy away `quadtree.hpp`.

### Usage

```cpp
// Suppose this is the object to manage.
struct Object {...};

// Stops to split the leaf node when:
// 1. its width and height are both not larger than 2.
// 2. or the number of managed objects not larger than 2.
quadtree::SplitingStopper ssf = [](int w, int h, int n) {
  return (w <= 2 && h <= 2) || (n <= 2);
};

// Creates a tree on a 30x30 grids region.
quadtree::Quadtree<Object*> tree(30, 30, ssf);

// Build the tree.
tree.Build();

// Add a pointer to object obj1 locates at position (3,4)
tree.Add(3, 4, obj1);

// Remove the pointer to object obj1 locates at position (3,4)
tree.Add(3, 4, obj1);

// Query the objects inside rectangle [(x1,y1), (x2,y2)]
tree.QueryRange(x1, y1, x2, y2, [](int x, int y, Object* o) {
  // Handle object o locating at position (x, y)
});

// Visit every leaf nodes.
tree.ForEachNode([](quadtree::NodeId id, quadtree::Node<Object*>* node) {
  if (node->isLeaf) {
    // if node is a leaf, visit its objects
    for (auto [x, y, o]: node->objects) {
      // for every object inside this leaf node
      // object o locates at position (x,y)
    }
  }
});
```

### License

BSD
