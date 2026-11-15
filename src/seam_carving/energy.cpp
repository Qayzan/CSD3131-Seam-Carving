// ============================================================================
// energy.cpp  --  see energy.hpp
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation).
// ============================================================================
#include "energy.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "image_utils.hpp"

namespace {

// Everything one pass needs: the greyscale picture and, for every row and
// column, the index of its neighbour on each side after reflection. Looking
// the neighbours up in a table keeps the inner loop free of border tests.
struct SobelInput {
    int rows = 0;
    int cols = 0;
    std::vector<double> gray;           // rows * cols luminance values
    std::vector<int> up, down;          // up[r] = row above r, down[r] = row below r
    std::vector<int> left, right;       // left[c], right[c]

    explicit SobelInput(const Image& image)
        : rows(image.rows), cols(image.cols), gray(toGrayscale(image)),
          up(rows), down(rows), left(cols), right(cols) {
        for (int r = 0; r < rows; ++r) {
            up[r] = reflect101(r - 1, rows);
            down[r] = reflect101(r + 1, rows);
        }
        for (int c = 0; c < cols; ++c) {
            left[c] = reflect101(c - 1, cols);
            right[c] = reflect101(c + 1, cols);
        }
    }

    double g(int r, int c) const {
        return gray[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                    static_cast<std::size_t>(c)];
    }

    // |Gx| + |Gy| at (r, c), straight from the 3x3 kernels in energy.hpp.
    double energyAt(int r, int c) const {
        const int ru = up[r], rd = down[r], cl = left[c], cr = right[c];
        const double gx = (g(ru, cr) - g(ru, cl)) + 2.0 * (g(r, cr) - g(r, cl)) +
                          (g(rd, cr) - g(rd, cl));
        const double gy = (g(rd, cl) - g(ru, cl)) + 2.0 * (g(rd, c) - g(ru, c)) +
                          (g(rd, cr) - g(ru, cr));
        return std::fabs(gx) + std::fabs(gy);
    }
};

EnergyMap emptyMap(int rows, int cols) {
    EnergyMap map;
    map.rows = rows;
    map.cols = cols;
    map.values.assign(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols), 0.0);
    return map;
}

// Filters the block of rows [r0, r1) x columns [c0, c1) directly.
void filterBlock(const SobelInput& in, EnergyMap& out, int r0, int r1, int c0, int c1) {
    for (int r = r0; r < r1; ++r)
        for (int c = c0; c < c1; ++c) out.at(r, c) = in.energyAt(r, c);
}

// Divide: halve the longer side. Conquer: recurse. Combine: nothing to do --
// each half writes its own part of `out`, and the halo pixels it reads are
// only read, never written, so the halves are independent.
void divideAndConquer(const SobelInput& in, EnergyMap& out, int r0, int r1, int c0, int c1,
                      long long leafPixels) {
    const int h = r1 - r0, w = c1 - c0;
    if (static_cast<long long>(h) * w <= leafPixels) {
        filterBlock(in, out, r0, r1, c0, c1);
    } else if (h >= w) {
        const int mid = r0 + h / 2;
        divideAndConquer(in, out, r0, mid, c0, c1, leafPixels);
        divideAndConquer(in, out, mid, r1, c0, c1, leafPixels);
    } else {
        const int mid = c0 + w / 2;
        divideAndConquer(in, out, r0, r1, c0, mid, leafPixels);
        divideAndConquer(in, out, r0, r1, mid, c1, leafPixels);
    }
}

}  // namespace

int reflect101(int i, int n) {
    if (n <= 1) return 0;
    if (i < 0) return -i;              // -1 -> 1
    if (i >= n) return 2 * n - 2 - i;  // n -> n - 2
    return i;
}

EnergyMap sobelEnergy(const Image& image) {
    checkImage(image, "sobelEnergy");
    const SobelInput in(image);
    EnergyMap out = emptyMap(in.rows, in.cols);
    filterBlock(in, out, 0, in.rows, 0, in.cols);
    return out;
}

EnergyMap sobelEnergySeparable(const Image& image) {
    checkImage(image, "sobelEnergySeparable");
    const SobelInput in(image);
    const int rows = in.rows, cols = in.cols;
    // Pass 1 (along each row): difference d = right - left, smoothing s = left + 2*centre + right.
    EnergyMap d = emptyMap(rows, cols), s = emptyMap(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            d.at(r, c) = in.g(r, in.right[c]) - in.g(r, in.left[c]);
            s.at(r, c) = in.g(r, in.left[c]) + 2.0 * in.g(r, c) + in.g(r, in.right[c]);
        }
    // Pass 2 (down each column): Gx smooths d with (1, 2, 1); Gy differentiates s with (-1, 0, 1).
    EnergyMap out = emptyMap(rows, cols);
    for (int r = 0; r < rows; ++r) {
        const int ru = in.up[r], rd = in.down[r];
        for (int c = 0; c < cols; ++c) {
            const double gx = d.at(ru, c) + 2.0 * d.at(r, c) + d.at(rd, c);
            const double gy = s.at(rd, c) - s.at(ru, c);
            out.at(r, c) = std::fabs(gx) + std::fabs(gy);
        }
    }
    return out;
}

EnergyMap sobelEnergyDivideAndConquer(const Image& image, int leafPixels) {
    checkImage(image, "sobelEnergyDivideAndConquer");
    if (leafPixels < 1)
        throw std::invalid_argument("sobelEnergyDivideAndConquer: leafPixels must be >= 1");
    const SobelInput in(image);
    EnergyMap out = emptyMap(in.rows, in.cols);
    divideAndConquer(in, out, 0, in.rows, 0, in.cols, leafPixels);
    return out;
}

EnergyMap transpose(const EnergyMap& energy) {
    checkEnergyMap(energy, "transpose");
    EnergyMap out = emptyMap(energy.cols, energy.rows);
    for (int r = 0; r < energy.rows; ++r)
        for (int c = 0; c < energy.cols; ++c) out.at(c, r) = energy.at(r, c);
    return out;
}
