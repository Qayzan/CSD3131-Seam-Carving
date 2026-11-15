// ============================================================================
// dynamic_programming.hpp  --  minimum-energy seam by dynamic programming
// Owner: FF (Fauzan Fauzy), brief Task 1(b) (vertical AND horizontal seams).
//
// The idea in plain words. Walk down the picture one row at a time. For every
// pixel, remember the cheapest way to reach it from the top row:
//
//   M[0][j] = E[0][j]                                            (base case)
//   M[i][j] = E[i][j] + min( M[i-1][j-1], M[i-1][j], M[i-1][j+1] )  (i >= 1)
//             (a neighbour outside the picture counts as +infinity)
//
// M[i][j] is the energy of the cheapest seam from row 0 down to pixel (i, j).
// It only needs the row above, so every one of the rows*cols sub-problems is
// solved ONCE (plain recursion would solve the same ones exponentially often).
// The cheapest seam ends at the smallest entry of the bottom row; following
// the remembered choices ("back-pointers") upwards gives the whole seam.
//
// Ties: "leftmost wins". Among equal predecessors the smallest column is
// chosen, and in the bottom row the smallest column among equal minima. That
// makes the result deterministic, and identical to the exhaustive test oracle.
//
// Horizontal seams: a horizontal seam of E is a vertical seam of E transposed,
// so findSeam transposes the map (one Theta(rows*cols) copy) and reuses the
// vertical code. Index j of the result is the ROW removed from column j.
//
// Cost for an H x W map: Theta(H*W) time (3 comparisons per cell) and
// Theta(H*W) memory for the tables. findSeamCompact keeps only two rows of M
// plus 1-byte back-pointers (about 8x less memory, same time and same seam):
// the "do we need the entire DP table?" improvement in the brief, item 5(d).
// ============================================================================
#ifndef DYNAMIC_PROGRAMMING_HPP
#define DYNAMIC_PROGRAMMING_HPP

#include <cstdint>
#include <vector>

#include "interfaces.hpp"

namespace dp {

// The full DP tables for VERTICAL seams of `energy`, kept for the report,
// the visualisation and the tests.
struct Table {
    int rows = 0, cols = 0;
    std::vector<double> cost;        // M, row by row: cost[i * cols + j] = M[i][j]
    std::vector<std::int8_t> back;   // -1, 0 or +1: the column step to the chosen pixel above
                                     // (row 0 has no predecessor and holds 0)
    double at(int r, int c) const { return cost[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(c)]; }
    int step(int r, int c) const { return back[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(c)]; }
};

// Fills M and the back-pointers (vertical seams). Throws std::invalid_argument
// for an empty or inconsistent map.
Table cumulativeEnergy(const EnergyMap& energy);

// Follows the back-pointers up from the leftmost minimum of the bottom row.
// Returns a vertical seam; its energy is M at the bottom of the seam.
Seam backtrack(const Table& table);

// The shared entry point (docs/interfaces.md): the minimum-energy seam in
// the given direction. Does not modify `energy`. Deterministic.
Seam findSeam(const EnergyMap& energy, Orientation orientation);

// Same seam as findSeam, using two rolling rows of M and an int8_t
// back-pointer table instead of the full table of doubles.
Seam findSeamCompact(const EnergyMap& energy, Orientation orientation);

}  // namespace dp

#endif  // DYNAMIC_PROGRAMMING_HPP
