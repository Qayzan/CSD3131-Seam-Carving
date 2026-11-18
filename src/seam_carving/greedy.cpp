// ============================================================================
// greedy.cpp  --  greedy seam finder (item 3). 
// Owner: HM (Lee Hwee Min).
// ============================================================================
#include "greedy.hpp"

#include <cstddef>
#include <stdexcept>

namespace greedy {

namespace {

// Walks a seam of `length` steps. At every step there are `width` positions.
//   Vertical seam:   a step is a row,    a position is a column.
//   Horizontal seam: a step is a column, a position is a row.
// `cost(step, pos)` returns the energy of that pixel. Writing the walk once
// and passing in `cost` lets both orientations share the same code without
// building a transposed copy of the energy map.
//
// Time: Theta(width) for the first step + Theta(1) for each later step
//       = Theta(width + length). Extra space: the seam itself, Theta(length).
template <typename CostFn>
Seam walkSeam(int length, int width, CostFn cost, Orientation orientation) {
    Seam seam;
    seam.orientation = orientation;
    seam.indices.resize(static_cast<std::size_t>(length));

    // Step 0: the cheapest pixel in the whole first row/column.
    // Strict '<' keeps the earliest index on ties (leftmost / topmost wins).
    int pos = 0;
    for (int p = 1; p < width; ++p) {
        if (cost(0, p) < cost(0, pos)) pos = p;
    }
    seam.indices[0] = pos;
    double total = cost(0, pos);

    // Later steps: only the neighbours pos-1, pos, pos+1 are allowed (that
    // keeps the seam connected). Clamp them at the image border.
    for (int step = 1; step < length; ++step) {
        int lo = (pos > 0) ? pos - 1 : pos;
        int hi = (pos < width - 1) ? pos + 1 : pos;

        int best = lo;
        for (int p = lo + 1; p <= hi; ++p) {
            if (cost(step, p) < cost(step, best)) best = p;
        }

        pos = best;
        seam.indices[static_cast<std::size_t>(step)] = pos;
        total += cost(step, pos);
    }

    seam.energy = total;
    return seam;
}

}  // namespace

Seam findSeam(const EnergyMap& energy, Orientation orientation) {
    const std::size_t expected =
        static_cast<std::size_t>(energy.rows) * static_cast<std::size_t>(energy.cols);
    if (energy.rows < 1 || energy.cols < 1 || energy.values.size() != expected) {
        throw std::invalid_argument(
            "greedy::findSeam: energy map is empty or values.size() != rows * cols");
    }

    if (orientation == Orientation::Vertical) {
        // One step per row; choose a column in each row.
        return walkSeam(energy.rows, energy.cols,
                        [&energy](int row, int col) { return energy.at(row, col); },
                        orientation);
    }

    // Horizontal: one step per column; choose a row in each column.
    return walkSeam(energy.cols, energy.rows,
                    [&energy](int col, int row) { return energy.at(row, col); },
                    orientation);
}

}  // namespace greedy