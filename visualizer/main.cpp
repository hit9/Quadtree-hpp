#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

#include <argparse/argparse.hpp>
#include <unordered_set>

#include "quadtree.hpp"

// pixels per grid side.
const int GRID_SIZE = 24;

// Max value of w and h.
const int N = 80;

// Grids to render, 0 for empty, 1 for object.
int GRIDS[N][N];

// QUERY_ANSWER[x][y] => true means the object hits the range query.
int QUERY_ANSWER[N][N];

struct Options {
  // Width and height of the large rectangle region.
  int w = 10, h = 10;
  // Max number of objects inside a single leaf node.
  int max_number_objects_inside_leaf_node = 1;
  // Milliseconds between SDL frames.
  int delay_ms = 50;
  // Use ssf1
  bool use_ssf1 = false;
};

// Parse options from command line.
// Returns 0 on success.
int ParseOptionsFromCommandline(int argc, char* argv[], Options& options);

class Visualizer {
 public:
  Visualizer(quadtree::Quadtree<int>& tree, Options& options);
  int Init();
  void Start();
  void Destroy();

 private:
  quadtree::Quadtree<int>& tree;
  Options& options;
  SDL_Window* window;
  SDL_Renderer* renderer;

  // Query range ((qx1,qy1), (qx2,qy2))
  int qx1 = -1, qy1 = -1, qx2 = -1, qy2 = -1;
  // qflag = 0: clear the range query, no query now.
  // qflag = 1: got left-upper corner input.
  // qflag = 2: got right-bottom corner input.
  int qflag = 0;

  // Query neighbour.
  // qnflag = 0: no query now.
  // qnflag = 1: start quering neighbour.
  // qnflag = 2: got node input now.
  // qnflag = 3: got the direction to query
  int qnflag = 0;
  // node to query.
  quadtree::Node<int>* qnNode = nullptr;
  // direction to query
  int qnDirection = 0;
  // answers of query neighbours (on qnflag = 3)
  std::unordered_set<quadtree::Node<int>*> qnAns;
  quadtree::Visitor<int> qnVisitor;

  void draw();
  int handleInputs();
  void clearQueryRange();
  void clearQueryNeighbours();
};

int main(int argc, char* argv[]) {
  memset(GRIDS, 0, sizeof GRIDS);

  // Parse arguments.
  Options options;
  if (ParseOptionsFromCommandline(argc, argv, options) != 0) return -1;
  // Quadtree.
  quadtree::SplitingStopper ssf = [&options](int w, int h, int n) {
    if (options.use_ssf1) return n == 0 || (w * h == n);
    return (w <= 2 && h <= 2) || (n <= options.max_number_objects_inside_leaf_node);
  };
  quadtree::Quadtree<int> tree(options.w, options.h, ssf);
  // Visualizer
  Visualizer visualizer(tree, options);
  if (visualizer.Init() != 0) return -1;
  visualizer.Start();
  visualizer.Destroy();
  return 0;
}

// ~~~~~~~~~~ Implementations ~~~~~~~~~~~~~~~

int ParseOptionsFromCommandline(int argc, char* argv[], Options& options) {
  argparse::ArgumentParser program("quadtree-visualizer");
  program.add_argument("-d", "--delay-ms")
      .help("milliseconds between SDL frames")
      .default_value(50)
      .store_into(options.delay_ms);
  program.add_argument("-w", "--width")
      .help("width of the whole rectangle region (number of grids)")
      .default_value(10)
      .store_into(options.w);
  program.add_argument("-h", "--height")
      .help("height of the whole rectangle region (number of grids)")
      .default_value(10)
      .store_into(options.h);
  program.add_argument("-k", "--max-number-objects-inside-leaf-node")
      .help("max number of obhects inside a leaf node")
      .default_value(1)
      .store_into(options.max_number_objects_inside_leaf_node);
  program.add_argument("-ssf1").help("use ssf1").default_value(false).store_into(options.use_ssf1);
  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& e) {
    spdlog::error(e.what());
    return 1;
  }
  if (options.w > N || options.h > N) {
    spdlog::error("w or h is too large to render a window");
    return 2;
  }
  return 0;
}

Visualizer::Visualizer(quadtree::Quadtree<int>& tree, Options& options)
    : tree(tree), options(options) {
  qnVisitor = [this](quadtree::Node<int>* node) -> void { qnAns.insert(node); };
}

int Visualizer::Init() {
  // Init SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    spdlog::error("SDL init error: {}", SDL_GetError());
    return -1;
  }
  // Creates window.
  int window_w = options.w * GRID_SIZE;
  int window_h = options.h * GRID_SIZE;
  window = SDL_CreateWindow("quadtree visualizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            window_w, window_h, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    spdlog::error("Create window error: {}", SDL_GetError());
    SDL_Quit();
    return -3;
  }
  // Creates renderer.
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (renderer == nullptr) {
    spdlog::error("Create renderer error: {}", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }
  // Build the tree.
  spdlog::info("Visualizer init done");
  tree.Build();
  spdlog::info("quadtree build done");
  return 0;
}

void Visualizer::Destroy() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Visualizer::clearQueryRange() {
  qflag = 0;
  qx1 = qy1 = qx2 = qy2 = -1;
  memset(QUERY_ANSWER, 0, sizeof QUERY_ANSWER);
  spdlog::info("Clear the range query");
}

void Visualizer::clearQueryNeighbours() {
  qnflag = 0;
  qnNode = nullptr;
  qnDirection = 0;
  qnAns.clear();
  spdlog::info("Clear the neighbour query");
}

void Visualizer::Start() {
  while (true) {
    // quit on -1
    if (handleInputs() == -1) break;

    // Background: white
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    draw();
    SDL_RenderPresent(renderer);

    SDL_Delay(options.delay_ms);
  }
}

int Visualizer::handleInputs() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:  // quit
        return -1;
      case SDL_KEYDOWN:  // ESC to quit
        if (e.key.keysym.sym == SDLK_ESCAPE) {
          clearQueryNeighbours();
          clearQueryRange();
          spdlog::info("ESC : clear all queries...");
          return 0;
        }
        if (e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL) {  // Ctrl-C
          spdlog::info("Ctrl-C : quit...");
          return -1;
        }
        if (e.key.keysym.sym == SDLK_n) {
          if (qnflag == 0) {
            spdlog::info("'n' is pressed, start quering neighbour, click mouse to select a node");
            qnflag = 1;
          } else {
            clearQueryNeighbours();
          }
        }
        if (qnflag == 2) {
          switch (e.key.keysym.sym) {
            case SDLK_0:
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
              qnDirection = e.key.keysym.sym - '0';
              // Run the query. TODO
              qnAns.clear();
              tree.FindNeighbourLeafNodes(qnNode, qnDirection, qnVisitor);
              qnflag = 3;
              spdlog::info("Query the neighbour on the direction {} done, {} neighbours",
                           qnDirection, qnAns.size());
              break;
            default:
              clearQueryNeighbours();
              spdlog::info("Unknown direction input, the neighbour query cleared!");
              break;
          }
        }
        break;
      case SDL_MOUSEBUTTONDOWN:

        if (e.button.button == SDL_BUTTON_LEFT) {
          if (qnflag != 0 && qnflag != 1) {
            clearQueryNeighbours();
            return 0;
          }
          // SDL:
          // x is the horizonal direction,
          // y is the vertical direction.
          int x = e.button.y / GRID_SIZE, y = e.button.x / GRID_SIZE;
          if (qnflag == 1) {  // Quering neighbour.
            qnNode = tree.Find(x, y);
            if (qnNode == nullptr) {  // failure, won't happen
              spdlog::warn("selected node is not found, back to normal");
              qnflag = 0;
            } else {
              spdlog::info(
                  "selected node to query neighbours, please press a direction number. (0~7)");
              qnflag = 2;
            }
          } else {  // Add or Remove objects.
            GRIDS[x][y] ^= 1;
            if (GRIDS[x][y]) {  // added a object
              tree.Add(x, y, 1);
              spdlog::info(
                  "Mouse left button clicked, added a object, number of leaf nodes: {}, depth: {}",
                  tree.NumLeafNodes(), tree.Depth());
            } else {
              tree.Remove(x, y, 1);
              spdlog::info(
                  "Mouse left button clicked, removed a object, number of leaf nodes: {}, depth: "
                  "{}",
                  tree.NumLeafNodes(), tree.Depth());
            }
          }
        } else if (e.button.button == SDL_BUTTON_RIGHT) {
          if (qnflag != 0) {
            clearQueryNeighbours();
            return 0;
          }

          // Query a range.
          qflag = (++qflag) % 3;
          int x = e.button.y / GRID_SIZE, y = e.button.x / GRID_SIZE;
          switch (qflag) {
            case 0:
              clearQueryRange();
              break;
            case 1:
              qx1 = x;
              qy1 = y;
              spdlog::info("Set query range rectangle left-upper corner");
              break;
            case 2:
              qx2 = x;
              qy2 = y;
              spdlog::info("Set query range rectangle right-bottom corner");
              // Ensure the qx2 >= qx1 and qy2 >= qy1
              if (!(qx1 <= qx2 && qy1 <= qy2 && qx1 >= 0 && qy1 >= 0)) {
                spdlog::info("Invalid Range! Reset!");
                clearQueryRange();
              } else {
                // Run the query.
                tree.QueryRange(qx1, qy1, qx2, qy2,
                                [](int x, int y, int o) { QUERY_ANSWER[x][y] = 1; });
                spdlog::info("Qange query answered done.");
              }
              break;
          }
        }
        break;
    }
  }
  return 0;
}

// Colors for rendering leaf node background.
const SDL_Color colors[17] = {
    {255, 128, 128, 255},  // light red
    {128, 255, 128, 255},  // light green
    {128, 128, 255, 255},  // light blue
    {255, 255, 128, 255},  // light yellow
    {128, 255, 255, 255},  // light cyan
    {255, 128, 255, 255},  // light magenta
    {255, 200, 128, 255},  // light orange
    {200, 128, 255, 255},  // light purple
    {173, 216, 230, 255},  // light blue 2
    {144, 238, 144, 255},  // light green 2
    {255, 255, 224, 255},  // light yellow 2
    {255, 182, 193, 255},  // light pink 2
    {221, 160, 221, 255},  // light purple 2
    {255, 224, 189, 255},  // light orange 2
    {175, 238, 238, 255},  // light cyan 2
    {211, 211, 211, 255},  // light gray 2
    {135, 206, 235, 255},  // light blue 3
};

void Visualizer::draw() {
  // Draw leaf node's rectangles background.
  quadtree::Visitor<int> visitor1 = [this](quadtree::Node<int>* node) -> void {
    if (node->isLeaf) {
      // SDL coordinates
      int x = node->y1 * GRID_SIZE;
      int y = node->x1 * GRID_SIZE;
      int w = (node->y2 - node->y1 + 1) * GRID_SIZE;
      int h = (node->x2 - node->x1 + 1) * GRID_SIZE;
      SDL_Rect rect = {x, y, w, h};
      auto sharing = quadtree::pack(node->d, node->x1, node->y1, options.w, options.h) + node->d;
      auto [r, g, b, a] = colors[sharing % 17];
      SDL_SetRenderDrawColor(renderer, r, g, b, a);
      SDL_RenderFillRect(renderer, &rect);
    }
  };
  tree.ForEachNode(visitor1);

  // Draw the grid lines.
  for (int i = 0, y = 0; i < options.h; i++, y += GRID_SIZE) {
    for (int j = 0, x = 0; j < options.w; j++, x += GRID_SIZE) {
      SDL_Rect rect = {x, y, GRID_SIZE, GRID_SIZE};
      SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);  // light gray
      SDL_RenderDrawRect(renderer, &rect);
      if (GRIDS[i][j]) {
        // Draw the object (black)
        SDL_Rect inner = {x + 1, y + 1, GRID_SIZE - 2, GRID_SIZE - 2};
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);  // gray
        SDL_RenderFillRect(renderer, &inner);
      }
    }
  }

  // Draw Queried neighbours
  quadtree::Visitor<int> visitor3 = [this](quadtree::Node<int>* node) -> void {
    if (node->isLeaf && qnAns.find(node) != qnAns.end()) {
      // SDL coordinates
      int x = node->y1 * GRID_SIZE;
      int y = node->x1 * GRID_SIZE;
      int w = (node->y2 - node->y1 + 1) * GRID_SIZE;
      int h = (node->x2 - node->x1 + 1) * GRID_SIZE;
      SDL_Rect rect = {x, y, w, h};
      // neighbour background red.
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      SDL_RenderFillRect(renderer, &rect);
    }
  };
  if (qnflag == 3) tree.ForEachNode(visitor3);

  // Draw leaf node's border line.
  quadtree::Visitor<int> visitor2 = [this](quadtree::Node<int>* node) -> void {
    if (node->isLeaf) {
      // SDL coordinates
      int x = node->y1 * GRID_SIZE;
      int y = node->x1 * GRID_SIZE;
      int w = (node->y2 - node->y1 + 1) * GRID_SIZE;
      int h = (node->x2 - node->x1 + 1) * GRID_SIZE;
      // Outer liner rectangle (border width 2)
      SDL_Rect rect1 = {x, y, w, h};
      SDL_Rect rect2 = {x + 1, y + 1, w - 2, h - 2};
      // red border for node to query.
      if (qnflag != 0 && node == qnNode)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // red
      else
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // black
      SDL_RenderDrawRect(renderer, &rect1);
      SDL_RenderDrawRect(renderer, &rect2);
    }
  };
  tree.ForEachNode(visitor2);

  // Draw the query range.
  if (qflag == 1 || qflag == 2) {
    // dark blue highlights the left corner.
    int x = qy1 * GRID_SIZE, y = qx1 * GRID_SIZE;
    SDL_Rect rect = {x, y, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);  // dark blue
    SDL_RenderFillRect(renderer, &rect);
  }
  if (qflag == 2) {
    // dark blue highlights the right corner.
    int x = qy2 * GRID_SIZE, y = qx2 * GRID_SIZE;
    SDL_Rect rect = {x, y, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);  // dark blue
    SDL_RenderFillRect(renderer, &rect);

    // highlights the query range (border 3).
    SDL_Rect query_range_rect1 = {qy1 * GRID_SIZE, qx1 * GRID_SIZE, (qy2 - qy1 + 1) * GRID_SIZE,
                                  (qx2 - qx1 + 1) * GRID_SIZE};
    SDL_Rect query_range_rect2 = {query_range_rect1.x + 1, query_range_rect1.y + 1,
                                  query_range_rect1.w - 2, query_range_rect1.h - 2};
    SDL_Rect query_range_rect3 = {query_range_rect1.x + 2, query_range_rect1.y + 2,
                                  query_range_rect1.w - 4, query_range_rect1.h - 4};
    SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);  // dark blue
    SDL_RenderDrawRect(renderer, &query_range_rect1);
    SDL_RenderDrawRect(renderer, &query_range_rect2);
    SDL_RenderDrawRect(renderer, &query_range_rect3);

    // And highlights the answer.
    for (int i = 0, y = 0; i < options.h; i++, y += GRID_SIZE) {
      for (int j = 0, x = 0; j < options.w; j++, x += GRID_SIZE) {
        if (QUERY_ANSWER[i][j]) {
          // highlights the hit (green)
          SDL_Rect inner = {x + 1, y + 1, GRID_SIZE - 2, GRID_SIZE - 2};
          SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // gray
          SDL_RenderFillRect(renderer, &inner);
        }
      }
    }
  }
}
