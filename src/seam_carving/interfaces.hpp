// ============================================================================
// interfaces.hpp  --  the shared types every part of the project uses
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation).
//
// This file is the "contract" from docs/interfaces.md. It lets the DP (YN),
// greedy (HM) and graph-cut (KO/WN) seam finders be written independently and
// still plug into the same carver, tests, benchmarks and demo app.
//
// Conventions (all 0-based, row 0 = top, column 0 = left):
//   * Image      rows x cols x channels bytes, row by row, channels interleaved.
//                channels = 1 (grey) or 3 (RGB, in that order).
//   * EnergyMap  rows x cols doubles, row by row, every value >= 0.
//   * Seam       Vertical:   one column per ROW    (indices.size() == rows)
//                Horizontal: one row per COLUMN    (indices.size() == cols)
//                Neighbouring indices differ by at most 1 (8-connected path).
//
// The structs are plain aggregates (no constructors), so code such as
//     EnergyMap map; map.rows = 2; map.cols = 3; map.values = {...};
// keeps working. They are compatible with the temporary placeholder that
// HM's greedy branch used.
// ============================================================================
#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Orientation of a seam.
// ---------------------------------------------------------------------------
enum class Orientation {
    Vertical,    // top -> bottom; removes one pixel from every ROW (width - 1)
    Horizontal,  // left -> right; removes one pixel from every COLUMN (height - 1)
};

inline const char* toString(Orientation orientation) {
    return orientation == Orientation::Vertical ? "vertical" : "horizontal";
}

// ---------------------------------------------------------------------------
// An 8-bit image. pixels[(r * cols + c) * channels + k] is channel k of (r, c).
// ---------------------------------------------------------------------------
struct Image {
    int rows = 0;
    int cols = 0;
    int channels = 0;                  // 1 (grey) or 3 (RGB)
    std::vector<std::uint8_t> pixels;  // rows * cols * channels values

    std::size_t index(int r, int c, int k = 0) const {
        return (static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                static_cast<std::size_t>(c)) *
                   static_cast<std::size_t>(channels) +
               static_cast<std::size_t>(k);
    }
    std::uint8_t at(int r, int c, int k = 0) const { return pixels[index(r, c, k)]; }
    std::uint8_t& at(int r, int c, int k = 0) { return pixels[index(r, c, k)]; }
};

// ---------------------------------------------------------------------------
// Energy (importance) of every pixel. values[r * cols + c] = energy of (r, c).
// One flat vector instead of vector<vector<double>>: one block of memory,
// cache-friendly row-by-row loops, cheap to copy.
// ---------------------------------------------------------------------------
struct EnergyMap {
    int rows = 0;
    int cols = 0;
    std::vector<double> values;

    std::size_t index(int r, int c) const {
        return static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
               static_cast<std::size_t>(c);
    }
    double at(int r, int c) const { return values[index(r, c)]; }
    double& at(int r, int c) { return values[index(r, c)]; }
};

// ---------------------------------------------------------------------------
// A seam and its total energy.
// ---------------------------------------------------------------------------
struct Seam {
    Orientation orientation = Orientation::Vertical;
    std::vector<int> indices;
    // Vertical:   indices.size() == rows, indices[i] = column removed in row i
    // Horizontal: indices.size() == cols, indices[j] = row removed in column j
    double energy = 0.0;  // sum of the energy map along the seam (the map the finder received)
};

// ---------------------------------------------------------------------------
// Every seam-finding algorithm has this shape, e.g. greedy::findSeam.
// Rules (docs/interfaces.md): do not modify the map (it is const); return a
// valid seam; Seam::energy = sum along the seam; deterministic; no library
// seam-carving functions.
// ---------------------------------------------------------------------------
using SeamFinder = std::function<Seam(const EnergyMap&, Orientation)>;

// Energy function used by the carver (default: sobelEnergy in energy.hpp).
using EnergyFunction = std::function<EnergyMap(const Image&)>;

// ---------------------------------------------------------------------------
// Results of a full resize (carve() in carver.hpp).
// ---------------------------------------------------------------------------
enum class CarveOrder {
    WidthFirst,   // all vertical seams, then all horizontal seams (default)
    HeightFirst,  // all horizontal seams, then all vertical seams
    Alternate,    // vertical, horizontal, vertical, ... while both still shrink
};

struct Timings {        // seconds, summed over all seams
    double energy = 0.0;    // computing the energy maps
    double findSeam = 0.0;  // the seam finder only (fair comparison between algorithms)
    double remove = 0.0;    // validating + removing seams
    double total = 0.0;     // the whole carve() call
};

struct CarveResult {
    Image image;              // resized image, same number of channels as the input
    std::vector<Seam> seams;  // removal order; each in the coordinates of the image at that step
    Timings timings;
    std::vector<Image> frames;  // if requested: frames[0] = input, frames[k] = after seam k
};

// ---------------------------------------------------------------------------
// Shape checks shared by every module. They throw std::invalid_argument with a
// message that names the problem, so bad input never reaches the algorithms.
// ---------------------------------------------------------------------------
inline void checkImage(const Image& image, const char* where = "image") {
    if (image.rows < 1 || image.cols < 1)
        throw std::invalid_argument(std::string(where) + ": image must be at least 1x1");
    if (image.channels != 1 && image.channels != 3)
        throw std::invalid_argument(std::string(where) +
                                    ": image must have 1 (grey) or 3 (RGB) channels");
    const std::size_t expected = static_cast<std::size_t>(image.rows) *
                                 static_cast<std::size_t>(image.cols) *
                                 static_cast<std::size_t>(image.channels);
    if (image.pixels.size() != expected)
        throw std::invalid_argument(std::string(where) +
                                    ": pixels.size() != rows * cols * channels");
}

inline void checkEnergyMap(const EnergyMap& energy, const char* where = "energy map") {
    if (energy.rows < 1 || energy.cols < 1)
        throw std::invalid_argument(std::string(where) + ": energy map must be at least 1x1");
    const std::size_t expected =
        static_cast<std::size_t>(energy.rows) * static_cast<std::size_t>(energy.cols);
    if (energy.values.size() != expected)
        throw std::invalid_argument(std::string(where) + ": values.size() != rows * cols");
}

#endif  // INTERFACES_HPP
