// ============================================================================
// carver.hpp  --  repeated resizing with any seam finder
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation, brief Task 1(c)).
//
// carve() shrinks an image by removing one seam at a time. For EVERY seam:
//
//     E    = energy(image)               recomputed: removing a seam brings
//                                         pixels together, so gradients change
//     seam = finder(E, orientation)      DP, greedy or graph cut -- the only
//                                         difference between the three methods
//     check the seam (orientation, shape, connectivity, Seam::energy)
//     image = removeSeam(image, seam)
//
// The energy function, the checks, the removal code and the timing are the
// same for every finder, so the comparison between algorithms (item 9) is fair.
// ============================================================================
#ifndef CARVER_HPP
#define CARVER_HPP

#include <utility>
#include <vector>

#include "energy.hpp"
#include "interfaces.hpp"

// The orientation of each seam to remove, in order. `columnsToRemove` vertical
// seams and `rowsToRemove` horizontal seams, arranged according to `order`.
std::vector<Orientation> removalPlan(int columnsToRemove, int rowsToRemove, CarveOrder order);

// Shrinks `image` to targetRows x targetCols.
//   finder      any SeamFinder (e.g. greedy::findSeam). Called once per seam with
//               the freshly computed energy map (passed by const reference).
//   order       WidthFirst (default), HeightFirst or Alternate.
//   keepFrames  if true, result.frames = { input, after seam 1, after seam 2, ... }.
//   energy      energy function, recomputed before every seam (default sobelEnergy).
//               A parameter so tests can count the calls and other energies can be tried.
// Returns the resized image, the seams in removal order (each in the coordinates
// of the image at that moment) and the time spent in each stage.
// Throws std::invalid_argument for an invalid image, a target that is < 1 or
// larger than the image (only shrinking is supported), an empty finder, or a
// finder result that breaks the contract (wrong orientation, invalid seam,
// wrong Seam::energy).
CarveResult carve(const Image& image, int targetRows, int targetCols, const SeamFinder& finder,
                  CarveOrder order = CarveOrder::WidthFirst, bool keepFrames = false,
                  const EnergyFunction& energy = sobelEnergy);

// Maps each removed seam back to (row, col) positions in the ORIGINAL image,
// by replaying the removals on a map that remembers every surviving pixel's
// original position. Used to draw all removed seams on the input picture
// (bonus 7a). `seams` must be a CarveResult's seams for an image of the given size.
std::vector<std::vector<std::pair<int, int>>> seamsInOriginalCoordinates(
    int originalRows, int originalCols, const std::vector<Seam>& seams);

#endif  // CARVER_HPP
