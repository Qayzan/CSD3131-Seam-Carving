// ============================================================================
// greedy.hpp  --  greedy seam finder (item 3, brief Task 3)
// Owner: HM (Lee Hwee Min).
//
// The greedy method builds a seam one step at a time and never looks ahead:
//   1. In the first row, take the pixel with the lowest energy.
//   2. In every following row, look only at the (up to) 3 pixels touching the
//      previous seam pixel -- directly below and the two diagonals -- and take
//      the cheapest one.
// Each choice is the best LOCAL choice, but an early cheap pixel can lead the
// seam into an expensive region later. Unlike DP, the result is therefore not
// guaranteed to be the minimum-energy seam (greedy energy >= DP energy).
//
// Tie rule (same as the DP, docs/algorithm_design.md): when several pixels have
// the same energy, the one with the smallest index (leftmost / topmost) wins.
// ============================================================================
#ifndef GREEDY_HPP
#define GREEDY_HPP

#include "interfaces.hpp"

namespace greedy {

// Finds one seam in `energy` using greedy, step-by-step choices.
//
// Vertical seam:   indices.size() == energy.rows, indices[r] = column removed in row r.
// Horizontal seam: indices.size() == energy.cols, indices[c] = row removed in column c.
// Seam::energy is the sum of the energy map along the returned seam.
// The energy map is only read, never changed.
//
// Throws std::invalid_argument if the map has no pixels or if
// values.size() != rows * cols.
Seam findSeam(const EnergyMap& energy, Orientation orientation);

}  // namespace greedy

#endif  // GREEDY_HPP