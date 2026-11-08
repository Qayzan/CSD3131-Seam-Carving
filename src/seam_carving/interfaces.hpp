// ============================================================================
// interfaces.hpp  --  TEMPORARY placeholder on feature/hm-greedy ONLY
// Do NOT open a pull request while this placeholder is still here.
// ============================================================================
#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <cstddef>
#include <vector>

enum class Orientation {
    Vertical,    // top -> bottom; removes one pixel from every ROW (width - 1)
    Horizontal,  // left -> right; removes one pixel from every COLUMN (height - 1)
};

struct EnergyMap {
    int rows = 0, cols = 0;
    std::vector<double> values;  // values[r * cols + c] = energy of pixel (r, c)
    double at(int r, int c) const {
        return values[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                      static_cast<std::size_t>(c)];
    }
};

struct Seam {
    Orientation orientation = Orientation::Vertical;
    std::vector<int> indices;
    // Vertical:   indices.size() == rows, indices[i] = column removed in row i
    // Horizontal: indices.size() == cols, indices[j] = row removed in column j
    double energy = 0.0;  // sum of energy along the seam
};

#endif  // INTERFACES_HPP