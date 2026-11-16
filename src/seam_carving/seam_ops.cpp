// ============================================================================
// seam_ops.cpp  --  see seam_ops.hpp
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation).
// ============================================================================
#include "seam_ops.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "image_utils.hpp"

namespace {

// Removes the seam from any row-major grid of `channels` values per pixel.
// Both Image (uint8_t) and EnergyMap (double) use this, so the logic exists once.
//
// Vertical: copy each row, skipping one pixel.        One pass, Theta(rows * cols).
// Horizontal: walk the output in row-major order; in column c the source row is
// r if r is above the seam pixel, otherwise r + 1.     Also one pass.
template <typename T>
std::vector<T> removeFromGrid(const std::vector<T>& in, int rows, int cols, int channels,
                              const Seam& seam) {
    const std::size_t ch = static_cast<std::size_t>(channels);
    std::vector<T> out;
    if (seam.orientation == Orientation::Vertical) {
        out.resize(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols - 1) * ch);
        auto dst = out.begin();
        for (int r = 0; r < rows; ++r) {
            const auto rowStart = in.begin() + static_cast<std::ptrdiff_t>(
                                                   static_cast<std::size_t>(r) * cols * ch);
            const auto skip = rowStart + static_cast<std::ptrdiff_t>(
                                             static_cast<std::size_t>(seam.indices[r]) * ch);
            const auto rowEnd = rowStart + static_cast<std::ptrdiff_t>(cols * ch);
            dst = std::copy(rowStart, skip, dst);                                    // left of the seam
            dst = std::copy(skip + static_cast<std::ptrdiff_t>(ch), rowEnd, dst);  // right of it
        }
    } else {
        out.resize(static_cast<std::size_t>(rows - 1) * static_cast<std::size_t>(cols) * ch);
        std::size_t dst = 0;
        for (int r = 0; r < rows - 1; ++r) {
            for (int c = 0; c < cols; ++c) {
                const int source = (r < seam.indices[c]) ? r : r + 1;
                const std::size_t src =
                    (static_cast<std::size_t>(source) * cols + static_cast<std::size_t>(c)) * ch;
                for (std::size_t k = 0; k < ch; ++k) out[dst++] = in[src + k];
            }
        }
    }
    return out;
}

void checkRemovable(const Seam& seam, int rows, int cols) {
    validateSeam(seam, rows, cols);
    if (seam.orientation == Orientation::Vertical && cols == 1)
        throw std::invalid_argument("removeSeam: cannot remove a vertical seam from a 1-pixel-wide picture");
    if (seam.orientation == Orientation::Horizontal && rows == 1)
        throw std::invalid_argument("removeSeam: cannot remove a horizontal seam from a 1-pixel-tall picture");
}

}  // namespace

std::string seamProblem(const Seam& seam, int rows, int cols) {
    if (rows < 1 || cols < 1) return "the picture must be at least 1x1";
    const bool vertical = seam.orientation == Orientation::Vertical;
    const int length = vertical ? rows : cols;  // one entry per row (or per column)
    const int limit = vertical ? cols : rows;   // each entry must be below this
    const char* along = vertical ? "row" : "column";
    const char* across = vertical ? "column" : "row";
    std::ostringstream msg;
    if (static_cast<long long>(seam.indices.size()) != length) {
        msg << toString(seam.orientation) << " seam for a " << rows << "x" << cols
            << " picture needs " << length << " entries (one per " << along << "), got "
            << seam.indices.size();
        return msg.str();
    }
    for (int k = 0; k < length; ++k) {
        const int v = seam.indices[k];
        if (v < 0 || v >= limit) {
            msg << toString(seam.orientation) << " seam leaves the picture: " << along << " " << k
                << " has " << across << " " << v << ", valid " << across << "s are 0.."
                << limit - 1;
            return msg.str();
        }
        if (k > 0 && std::abs(v - seam.indices[k - 1]) > 1) {
            msg << toString(seam.orientation) << " seam is not connected: " << along << " "
                << k - 1 << " -> " << k << " jumps from " << across << " " << seam.indices[k - 1]
                << " to " << v << " (neighbours may differ by at most 1)";
            return msg.str();
        }
    }
    return "";
}

bool isValidSeam(const Seam& seam, int rows, int cols) {
    return seamProblem(seam, rows, cols).empty();
}

void validateSeam(const Seam& seam, int rows, int cols) {
    const std::string problem = seamProblem(seam, rows, cols);
    if (!problem.empty()) throw std::invalid_argument("invalid seam: " + problem);
}

double seamEnergy(const EnergyMap& energy, const Seam& seam) {
    checkEnergyMap(energy, "seamEnergy");
    validateSeam(seam, energy.rows, energy.cols);
    double sum = 0.0;  // summed in seam order (top -> bottom / left -> right)
    for (const auto& [r, c] : seamPixels(seam)) sum += energy.at(r, c);
    return sum;
}

void validateSeamEnergy(const Seam& seam, const EnergyMap& energy, double relativeTolerance) {
    const double expected = seamEnergy(energy, seam);
    if (!(std::fabs(seam.energy - expected) <=
          relativeTolerance * std::max(1.0, std::fabs(expected)))) {
        std::ostringstream msg;
        msg.precision(17);
        msg << "seam.energy = " << seam.energy << " but the energy along the seam is " << expected;
        throw std::invalid_argument(msg.str());
    }
}

std::vector<std::pair<int, int>> seamPixels(const Seam& seam) {
    std::vector<std::pair<int, int>> pixels;
    pixels.reserve(seam.indices.size());
    for (int k = 0; k < static_cast<int>(seam.indices.size()); ++k) {
        if (seam.orientation == Orientation::Vertical)
            pixels.emplace_back(k, seam.indices[k]);
        else
            pixels.emplace_back(seam.indices[k], k);
    }
    return pixels;
}

Image removeSeam(const Image& image, const Seam& seam) {
    checkImage(image, "removeSeam");
    checkRemovable(seam, image.rows, image.cols);
    Image out;
    out.rows = image.rows - (seam.orientation == Orientation::Horizontal ? 1 : 0);
    out.cols = image.cols - (seam.orientation == Orientation::Vertical ? 1 : 0);
    out.channels = image.channels;
    out.pixels = removeFromGrid(image.pixels, image.rows, image.cols, image.channels, seam);
    return out;
}

EnergyMap removeSeam(const EnergyMap& energy, const Seam& seam) {
    checkEnergyMap(energy, "removeSeam");
    checkRemovable(seam, energy.rows, energy.cols);
    EnergyMap out;
    out.rows = energy.rows - (seam.orientation == Orientation::Horizontal ? 1 : 0);
    out.cols = energy.cols - (seam.orientation == Orientation::Vertical ? 1 : 0);
    out.values = removeFromGrid(energy.values, energy.rows, energy.cols, 1, seam);
    return out;
}

Image highlightSeam(const Image& image, const Seam& seam, std::uint8_t red, std::uint8_t green,
                    std::uint8_t blue) {
    checkImage(image, "highlightSeam");
    validateSeam(seam, image.rows, image.cols);
    Image out = toRgb(image);
    for (const auto& [r, c] : seamPixels(seam)) {
        out.at(r, c, 0) = red;
        out.at(r, c, 1) = green;
        out.at(r, c, 2) = blue;
    }
    return out;
}
