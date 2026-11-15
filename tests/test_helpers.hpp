// ============================================================================
// test_helpers.hpp  --  shared test data and oracles (FF, test-only code)
//
// Nothing here is part of the library. The exhaustive seam search is an
// independent oracle: it tries EVERY seam, so it is obviously correct but only
// usable on tiny maps. Teammates can reuse it (DP and graph cut must match its
// energy exactly; greedy can only be >= it).
// ============================================================================
#ifndef SEAM_TEST_HELPERS_HPP
#define SEAM_TEST_HELPERS_HPP

#include <cstdint>
#include <random>
#include <vector>

#include "interfaces.hpp"

namespace testing_helpers {

inline EnergyMap makeMap(int rows, int cols, const std::vector<double>& values) {
    EnergyMap map;
    map.rows = rows;
    map.cols = cols;
    map.values = values;
    return map;
}

inline Image makeImageFrom(int rows, int cols, int channels, const std::vector<int>& values) {
    Image image;
    image.rows = rows;
    image.cols = cols;
    image.channels = channels;
    image.pixels.assign(values.begin(), values.end());
    return image;
}

inline Image randomImage(std::mt19937& rng, int rows, int cols, int channels) {
    std::uniform_int_distribution<int> byte(0, 255);
    Image image;
    image.rows = rows;
    image.cols = cols;
    image.channels = channels;
    image.pixels.resize(static_cast<std::size_t>(rows) * cols * channels);
    for (auto& p : image.pixels) p = static_cast<std::uint8_t>(byte(rng));
    return image;
}

// A random VALID seam: a random walk that moves -1, 0 or +1 per step.
inline Seam randomSeam(std::mt19937& rng, int rows, int cols, Orientation orientation) {
    const int length = orientation == Orientation::Vertical ? rows : cols;
    const int limit = orientation == Orientation::Vertical ? cols : rows;
    std::uniform_int_distribution<int> start(0, limit - 1), step(-1, 1);
    Seam seam;
    seam.orientation = orientation;
    seam.indices.resize(static_cast<std::size_t>(length));
    seam.indices[0] = start(rng);
    for (int k = 1; k < length; ++k) {
        int v = seam.indices[k - 1] + step(rng);
        if (v < 0) v = 0;
        if (v >= limit) v = limit - 1;
        seam.indices[k] = v;
    }
    return seam;
}

// Exhaustive search over every seam (oracle). Ties: the seam whose indices,
// read from the LAST row/column backwards, are smallest -- the tie rule the
// DP design uses (docs/algorithm_design.md 2.3). Energies are summed in seam
// order, top -> bottom, like the other finders.
struct BruteForce {
    const EnergyMap& energy;
    bool vertical;
    int length, width;
    std::vector<int> path;
    Seam best;
    bool found = false;

    double cost(int step, int pos) const { return vertical ? energy.at(step, pos) : energy.at(pos, step); }

    void evaluate() {
        double total = 0.0;
        for (int k = 0; k < length; ++k) total += cost(k, path[k]);
        bool better = !found || total < best.energy;
        if (found && total == best.energy) {
            for (int k = length - 1; k >= 0; --k)
                if (path[k] != best.indices[k]) {
                    better = path[k] < best.indices[k];
                    break;
                }
        }
        if (better) {
            best.indices = path;
            best.energy = total;
            found = true;
        }
    }

    void extend(int step) {  // path[0 .. step-1] is fixed; try every continuation
        if (step == length) {
            evaluate();
            return;
        }
        for (int d = -1; d <= 1; ++d) {
            const int next = path[step - 1] + d;
            if (next < 0 || next >= width) continue;
            path[step] = next;
            extend(step + 1);
        }
    }
};

inline Seam bruteForceSeam(const EnergyMap& energy, Orientation orientation) {
    const bool vertical = orientation == Orientation::Vertical;
    BruteForce search{energy, vertical, vertical ? energy.rows : energy.cols,
                      vertical ? energy.cols : energy.rows, {}, {}, false};
    search.best.orientation = orientation;
    search.path.resize(static_cast<std::size_t>(search.length));
    for (int start = 0; start < search.width; ++start) {
        search.path[0] = start;
        search.extend(1);
    }
    return search.best;
}

// Number of seams of a rows x cols grid (for checking the enumeration itself).
inline long long countVerticalSeams(int rows, int cols) {
    std::vector<long long> ways(static_cast<std::size_t>(cols), 1);
    for (int r = 1; r < rows; ++r) {
        std::vector<long long> next(static_cast<std::size_t>(cols), 0);
        for (int c = 0; c < cols; ++c)
            for (int d = -1; d <= 1; ++d)
                if (c + d >= 0 && c + d < cols) next[c] += ways[c + d];
        ways = next;
    }
    long long total = 0;
    for (long long w : ways) total += w;
    return total;
}

}  // namespace testing_helpers

#endif  // SEAM_TEST_HELPERS_HPP
