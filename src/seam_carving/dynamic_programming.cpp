// ============================================================================
// dynamic_programming.cpp  --  see dynamic_programming.hpp
// Owner: FF (Fauzan Fauzy), brief Task 1(b).
// ============================================================================
#include "dynamic_programming.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "energy.hpp"  // transpose(EnergyMap)

namespace dp {

namespace {

std::size_t cell(int r, int c, int cols) {
    return static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(c);
}

// The cheapest of the up-to-three pixels above column j, given the previous
// row of M. Writes the chosen step (-1, 0, +1) to `step`.
// Tie rule "leftmost wins": start with j, let j-1 win with <=, let j+1 win
// only with a strict <. So the smallest column among equal values is chosen.
double bestAbove(const double* above, int j, int cols, std::int8_t& step) {
    double best = above[j];
    step = 0;
    if (j > 0 && above[j - 1] <= best) {
        best = above[j - 1];
        step = -1;
    }
    if (j + 1 < cols && above[j + 1] < best) {
        best = above[j + 1];
        step = +1;
    }
    return best;
}

// Leftmost minimum of one row of M.
int leftmostMinimum(const double* row, int cols) {
    int best = 0;
    for (int j = 1; j < cols; ++j)
        if (row[j] < row[best]) best = j;
    return best;
}

// Walks the back-pointers up from column `end` of the bottom row.
std::vector<int> followBack(const std::vector<std::int8_t>& back, int rows, int cols, int end) {
    std::vector<int> seam(static_cast<std::size_t>(rows));
    seam[static_cast<std::size_t>(rows - 1)] = end;
    for (int i = rows - 1; i >= 1; --i) {
        const int j = seam[static_cast<std::size_t>(i)];
        seam[static_cast<std::size_t>(i - 1)] = j + back[cell(i, j, cols)];
    }
    return seam;
}

// Runs a vertical-seam finder on E (vertical) or on E transposed (horizontal).
template <typename VerticalFinder>
Seam inDirection(const EnergyMap& energy, Orientation orientation, const char* who,
                 VerticalFinder vertical) {
    checkEnergyMap(energy, who);
    if (orientation == Orientation::Vertical) return vertical(energy);
    Seam seam = vertical(transpose(energy));  // a row of E^T is a column of E
    seam.orientation = Orientation::Horizontal;
    return seam;
}

Seam verticalCompact(const EnergyMap& energy) {
    const int rows = energy.rows, cols = energy.cols;
    std::vector<double> above(energy.values.begin(), energy.values.begin() + cols);  // M[0] = E[0]
    std::vector<double> current(static_cast<std::size_t>(cols));
    std::vector<std::int8_t> back(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols), 0);
    for (int i = 1; i < rows; ++i) {
        for (int j = 0; j < cols; ++j)
            current[static_cast<std::size_t>(j)] =
                energy.at(i, j) + bestAbove(above.data(), j, cols, back[cell(i, j, cols)]);
        std::swap(above, current);  // only the last row of M is ever needed
    }
    const int end = leftmostMinimum(above.data(), cols);
    Seam seam;
    seam.orientation = Orientation::Vertical;
    seam.indices = followBack(back, rows, cols, end);
    seam.energy = above[static_cast<std::size_t>(end)];
    return seam;
}

}  // namespace

Table cumulativeEnergy(const EnergyMap& energy) {
    checkEnergyMap(energy, "dp::cumulativeEnergy");
    Table t;
    t.rows = energy.rows;
    t.cols = energy.cols;
    t.cost.resize(energy.values.size());
    t.back.assign(energy.values.size(), 0);
    for (int j = 0; j < t.cols; ++j) t.cost[cell(0, j, t.cols)] = energy.at(0, j);  // base case
    for (int i = 1; i < t.rows; ++i) {
        const double* above = &t.cost[cell(i - 1, 0, t.cols)];
        for (int j = 0; j < t.cols; ++j) {
            const std::size_t k = cell(i, j, t.cols);
            t.cost[k] = energy.at(i, j) + bestAbove(above, j, t.cols, t.back[k]);
        }
    }
    return t;
}

Seam backtrack(const Table& table) {
    if (table.rows < 1 || table.cols < 1 ||
        table.cost.size() != static_cast<std::size_t>(table.rows) * static_cast<std::size_t>(table.cols) ||
        table.back.size() != table.cost.size())
        throw std::invalid_argument("dp::backtrack: table is empty or its size does not match rows x cols");
    const int end = leftmostMinimum(&table.cost[cell(table.rows - 1, 0, table.cols)], table.cols);
    Seam seam;
    seam.orientation = Orientation::Vertical;
    seam.indices = followBack(table.back, table.rows, table.cols, end);
    seam.energy = table.at(table.rows - 1, end);
    return seam;
}

Seam findSeam(const EnergyMap& energy, Orientation orientation) {
    return inDirection(energy, orientation, "dp::findSeam",
                       [](const EnergyMap& e) { return backtrack(cumulativeEnergy(e)); });
}

Seam findSeamCompact(const EnergyMap& energy, Orientation orientation) {
    return inDirection(energy, orientation, "dp::findSeamCompact", verticalCompact);
}

}  // namespace dp
