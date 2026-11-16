// ============================================================================
// seam_ops.hpp  --  checking, measuring, removing and drawing seams
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation, brief Task 1(c)).
//
// The carver calls validateSeam after EVERY seam finder call, so a buggy
// finder fails loudly with a clear message instead of corrupting the image.
// ============================================================================
#ifndef SEAM_OPS_HPP
#define SEAM_OPS_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "interfaces.hpp"

// Empty string if `seam` is a legal seam for a rows x cols picture, otherwise
// a sentence saying what is wrong. Checks, in order:
//   1. length: rows for a vertical seam, cols for a horizontal one;
//   2. every index inside the picture;
//   3. connectivity: neighbouring indices differ by at most 1.
std::string seamProblem(const Seam& seam, int rows, int cols);

bool isValidSeam(const Seam& seam, int rows, int cols);

// Throws std::invalid_argument (with seamProblem's message) if the seam is not legal.
void validateSeam(const Seam& seam, int rows, int cols);

// Sum of the energy map along the seam. The seam must fit the map
// (std::invalid_argument otherwise).
double seamEnergy(const EnergyMap& energy, const Seam& seam);

// Throws std::invalid_argument unless seam.energy equals seamEnergy(energy, seam)
// within relativeTolerance * max(1, |true sum|) (contract rule 3).
void validateSeamEnergy(const Seam& seam, const EnergyMap& energy,
                        double relativeTolerance = 1e-9);

// (row, col) of every seam pixel, in seam order.
std::vector<std::pair<int, int>> seamPixels(const Seam& seam);

// New image with the seam removed:
//   vertical seam   -> rows x (cols - 1): in each row the pixels right of the seam move left;
//   horizontal seam -> (rows - 1) x cols: in each column the pixels below the seam move up.
// Every other pixel keeps its value and its order; the channel count is kept.
// Throws std::invalid_argument for an invalid seam, or if the picture is already
// 1 pixel wide (vertical) / 1 pixel tall (horizontal).
Image removeSeam(const Image& image, const Seam& seam);

// Same for an energy map (useful for teammates' tests and visualisations).
EnergyMap removeSeam(const EnergyMap& energy, const Seam& seam);

// RGB copy of the image with the seam's pixels painted in (red, green, blue).
// Grey images are turned into RGB first so the colour shows.
Image highlightSeam(const Image& image, const Seam& seam, std::uint8_t red = 255,
                    std::uint8_t green = 0, std::uint8_t blue = 0);

#endif  // SEAM_OPS_HPP
