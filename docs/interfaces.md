# Shared interface contract (DRAFT — FF finalises it in `interfaces.hpp`)

This is the agreement that lets YN, HM, WN and KO build their seam finders independently and still plug them into the same carver, tests, benchmarks and demo. FF owns the shared foundation (item 1) and writes the real types in `src/seam_carving/interfaces.hpp`. **Please review and comment before code depends on it.** After that, changes need a pull request reviewed by FF.

## 1. Data

| Thing | C++ type | Layout | Notes |
|---|---|---|---|
| Colour image | `Image`, 8-bit | `rows × cols × 3`, row by row, channels interleaved | **RGB** order |
| Greyscale image | `Image`, 8-bit | `rows × cols × 1` | accepted everywhere a colour image is |
| Energy map | `EnergyMap`, `double` | `rows × cols`, row by row | non-negative; produced by the Sobel energy (item 1) |

Indexing is always `[row][col]` (row `r`, column `c`). Row 0 is the top row and column 0 is the left column. The brief uses 1-based `img[1…m, 1…n]`. The code uses 0-based indices. The report says so once.

## 2. Types (to be implemented in `src/seam_carving/interfaces.hpp`)

```cpp
enum class Orientation {
    Vertical,    // top -> bottom; removes one pixel from every ROW (width - 1)
    Horizontal,  // left -> right; removes one pixel from every COLUMN (height - 1)
};

struct EnergyMap {
    int rows = 0, cols = 0;
    std::vector<double> values;          // values[r * cols + c] = energy of pixel (r, c)
    double at(int r, int c) const;
};

struct Image {
    int rows = 0, cols = 0, channels = 0;  // channels: 1 (grey) or 3 (RGB)
    std::vector<std::uint8_t> pixels;      // pixels[(r * cols + c) * channels + k]
};

struct Seam {
    Orientation orientation;
    std::vector<int> indices;
    // Vertical:   indices.size() == rows, indices[i] = column removed in row i
    // Horizontal: indices.size() == cols, indices[j] = row removed in column j
    double energy;  // sum of energy along the seam (same energy map the finder received)
};

using SeamFinder = std::function<Seam(const EnergyMap&, Orientation)>;
```

A flat `std::vector` stored row by row (instead of `vector<vector<>>`) keeps each map in one block of memory, which is faster and simpler to copy.

Rules every seam finder must follow:

1. It must **not modify** the energy map (it is passed as `const EnergyMap&`).
2. The returned seam must be **valid**: correct length; indices in range; consecutive indices differ by at most 1. `validateSeam` (in `seam_ops`) checks this, and the carver calls it after every finder call.
3. `Seam::energy` must equal the sum of the energy map along the seam (within floating-point tolerance).
4. Results must be deterministic for the same input.
5. It must not use any library content-aware resizing / seam-carving function (brief rule).

Each algorithm exposes one public entry point with this signature, in its own namespace:

```cpp
// dynamic_programming (YN), greedy (HM), graph_cut (KO, using WN's seam_graph + max_flow)
namespace dp       { Seam findSeam(const EnergyMap& energy, Orientation orientation); }
namespace greedy   { Seam findSeam(const EnergyMap& energy, Orientation orientation); }
namespace graphcut { Seam findSeam(const EnergyMap& energy, Orientation orientation); }
```

Algorithm-specific extras (the DP table M, the flow network, …) go in extra functions. They are not part of the shared signature.

## 3. Results of a full resize

```cpp
struct Timings { double energy, findSeam, remove, total; };  // seconds

struct CarveResult {
    Image image;               // resized image, same number of channels as the input
    std::vector<Seam> seams;   // in removal order, each in the coordinates of the image at that step
    Timings timings;
    std::vector<Image> frames; // optional intermediate images (visualisation / bonus 7a)
};

CarveResult carve(const Image& image, int targetRows, int targetCols, const SeamFinder& finder,
                  CarveOrder order = CarveOrder::WidthFirst, bool keepFrames = false);
```

- Throws `std::invalid_argument` if a target is larger than the current size, or less than 1.
- Recomputes the energy map before **every** seam (brief R1c).

## 4. Graph Cut hand-over (WN → KO)

Graph Cut is split in two (item 4). The part 1 interface already exists in `src/seam_carving/seam_graph.hpp` and `max_flow.hpp`:

```cpp
namespace graphcut {
SeamGraph buildVerticalSeamGraph(const Grid& energy);   // WN: the 4 kinds of edges
struct SeamGraph { FlowNetwork net; int rows, cols, source, sink; int node(int r, int c) const; };
ll FlowNetwork::maxFlow(int s, int t);                  // WN: Edmonds-Karp
const std::vector<Edge>& FlowNetwork::edges() const;    // residual capacities after maxFlow
}
```

KO's part 2 (`graph_cut.cpp`): after `maxFlow`, the source side S is every node reachable from `source` through edges with remaining capacity > 0. In row `i` the seam pixel is the rightmost column `j` with `node(i, j)` in S. Horizontal seams build the graph from `transpose(energy)`. `Grid` (`vector<vector<double>>`) is temporary: `buildVerticalSeamGraph` switches to `EnergyMap` once FF has written `interfaces.hpp`.

## 5. Timing

Timings use `std::chrono::steady_clock` around the finder call only, measured by the carver and the benchmark harness. This means the DP, greedy and graph cut are timed the same way, which matters for the comparison (item 9).

## 6. Questions for reviewers

- YN / HM / KO: is `findSeam(energy, orientation)` enough for your algorithm? Graph cut may prefer the image for forward energy. If so, we add an overload with an `Image` argument rather than changing the shared signature.
- FF: one flat `std::vector` per map, as above, or `vector<vector<>>`? Proposed: flat.
