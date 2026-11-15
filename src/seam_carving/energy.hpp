// ============================================================================
// energy.hpp  --  energy (importance) map with the 3x3 Sobel operator
// Owner: FF (Fauzan Fauzy), item 1 (energy map, brief Task 1(a)) and the
// measurements behind item 7 (divide and conquer for Sobel, Q2(b)).
//
// The energy of a pixel says how much it "stands out". Seam carving removes
// low-energy pixels first, so flat areas disappear before edges and texture.
//
//   e(r, c) = |Gx(r, c)| + |Gy(r, c)|          (Avidan & Shamir 2007, "e1")
//
// computed on the greyscale image I (double, BT.601 luminance) with the 3x3
// Sobel kernels, applied as a weighted sum over the neighbourhood of (r, c):
//
//          -1  0  +1                -1  -2  -1
//   Gx  =  -2  *  +2   on I    Gy =  0   *   0   on I      (* = pixel (r, c))
//          -1  0  +1                +1  +2  +1
//
// Gx measures the change from left to right, Gy from top to bottom. Sobel
// also averages across the other direction (weights 1, 2, 1), so a single
// noisy pixel counts less than with a plain neighbour difference.
//
// Borders ("reflect 101"): a missing neighbour is mirrored across the border
// WITHOUT repeating the edge pixel:  ... I[2] I[1] | I[0] I[1] I[2] ...
// (the same rule as OpenCV's default BORDER_REFLECT_101). A flat image
// therefore has energy 0 everywhere, including its border. Padding with zeros
// would make every border pixel look like a strong edge, and seams would
// never touch the border. For a 1-pixel-wide dimension the only neighbour is
// the pixel itself.
//
// Cost: Theta(rows * cols) time -- a fixed 9-pixel neighbourhood per pixel --
// and Theta(rows * cols) memory for the greyscale copy and the result.
// ============================================================================
#ifndef ENERGY_HPP
#define ENERGY_HPP

#include "interfaces.hpp"

// The energy map used by the whole project (and by default in carve()).
// Works for any image size down to 1x1, grey or RGB. Every value is >= 0.
EnergyMap sobelEnergy(const Image& image);

// ---- Alternative implementations (same result; item 7 discussion) ----------

// Separable Sobel: each 3x3 kernel is a product of two 1-D kernels,
//   Gx kernel = [1 2 1]^T (down the rows) x [-1 0 1] (across the columns)
//   Gy kernel = [-1 0 1]^T (down the rows) x [1 2 1] (across the columns)
// so each derivative takes two passes over 3 pixels instead of one pass over
// 9. Same Theta(rows * cols); results equal sobelEnergy up to rounding (1e-9).
EnergyMap sobelEnergySeparable(const Image& image);

// Divide and conquer: split the image into two halves (along its longer side)
// until a block has at most `leafPixels` pixels, then filter each block
// directly. A block reads a 1-pixel "halo" outside itself, so every pixel sees
// exactly the same 3x3 neighbourhood as in sobelEnergy and the results are
// bit-for-bit identical. Total work stays Theta(rows * cols) (see
// docs/theory/item7_divide_and_conquer_sobel.md).
// Throws std::invalid_argument if leafPixels < 1.
EnergyMap sobelEnergyDivideAndConquer(const Image& image, int leafPixels = 4096);

// ---- Helpers ---------------------------------------------------------------

// Mirror an out-of-range index into [0, n) using the reflect-101 rule:
// -1 -> 1, n -> n - 2 (and 0 when n == 1). Valid for i in [-1, n].
int reflect101(int i, int n);

// Swaps rows and columns: result.at(c, r) == energy.at(r, c).
// (Horizontal seams of a map = vertical seams of its transpose.)
EnergyMap transpose(const EnergyMap& energy);

#endif  // ENERGY_HPP
