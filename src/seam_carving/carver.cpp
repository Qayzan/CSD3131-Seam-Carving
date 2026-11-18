// ============================================================================
// carver.cpp  --  see carver.hpp
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation).
// ============================================================================
#include "carver.hpp"

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "seam_ops.hpp"

namespace {

using Clock = std::chrono::steady_clock;

double secondsSince(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

void checkTarget(const char* name, int target, int current) {
    if (target < 1)
        throw std::invalid_argument(std::string("carve: ") + name + " must be at least 1, got " +
                                    std::to_string(target));
    if (target > current)
        throw std::invalid_argument(std::string("carve: ") + name + " = " + std::to_string(target) +
                                    " is larger than the image (" + std::to_string(current) +
                                    "); only shrinking is supported");
}

}  // namespace

std::vector<Orientation> removalPlan(int columnsToRemove, int rowsToRemove, CarveOrder order) {
    if (columnsToRemove < 0 || rowsToRemove < 0)
        throw std::invalid_argument("removalPlan: counts must not be negative");
    std::vector<Orientation> plan;
    plan.reserve(static_cast<std::size_t>(columnsToRemove) + static_cast<std::size_t>(rowsToRemove));
    const auto add = [&plan](Orientation o, int count) { plan.insert(plan.end(), count, o); };
    switch (order) {
        case CarveOrder::WidthFirst:
            add(Orientation::Vertical, columnsToRemove);
            add(Orientation::Horizontal, rowsToRemove);
            break;
        case CarveOrder::HeightFirst:
            add(Orientation::Horizontal, rowsToRemove);
            add(Orientation::Vertical, columnsToRemove);
            break;
        case CarveOrder::Alternate:
            for (int k = 0; k < columnsToRemove || k < rowsToRemove; ++k) {
                if (k < columnsToRemove) plan.push_back(Orientation::Vertical);
                if (k < rowsToRemove) plan.push_back(Orientation::Horizontal);
            }
            break;
    }
    return plan;
}

CarveResult carve(const Image& image, int targetRows, int targetCols, const SeamFinder& finder,
                  CarveOrder order, bool keepFrames, const EnergyFunction& energy) {
    const Clock::time_point start = Clock::now();
    checkImage(image, "carve");
    checkTarget("targetRows", targetRows, image.rows);
    checkTarget("targetCols", targetCols, image.cols);
    if (!finder) throw std::invalid_argument("carve: no seam finder given");
    if (!energy) throw std::invalid_argument("carve: no energy function given");

    CarveResult result;
    result.image = image;
    if (keepFrames) result.frames.push_back(image);

    const std::vector<Orientation> plan =
        removalPlan(image.cols - targetCols, image.rows - targetRows, order);
    result.seams.reserve(plan.size());

    for (const Orientation orientation : plan) {
        // 1. Recompute the energy of the CURRENT image (brief Task 1(c)).
        Clock::time_point t = Clock::now();
        const EnergyMap map = energy(result.image);
        result.timings.energy += secondsSince(t);
        if (map.rows != result.image.rows || map.cols != result.image.cols)
            throw std::invalid_argument("carve: energy function returned a map of the wrong size");
        checkEnergyMap(map, "carve");

        // 2. Ask the finder for a seam (timed on its own for the comparison).
        t = Clock::now();
        Seam seam = finder(map, orientation);
        result.timings.findSeam += secondsSince(t);

        // 3. Check the finder kept the contract, then 4. remove the seam.
        t = Clock::now();
        if (seam.orientation != orientation)
            throw std::invalid_argument(std::string("carve: asked the finder for a ") +
                                        toString(orientation) + " seam, got a " +
                                        toString(seam.orientation) + " one");
        validateSeam(seam, map.rows, map.cols);
        validateSeamEnergy(seam, map);
        result.image = removeSeam(result.image, seam);
        result.timings.remove += secondsSince(t);

        result.seams.push_back(std::move(seam));
        if (keepFrames) result.frames.push_back(result.image);
    }
    result.timings.total = secondsSince(start);
    return result;
}

std::vector<std::vector<std::pair<int, int>>> seamsInOriginalCoordinates(
    int originalRows, int originalCols, const std::vector<Seam>& seams) {
    if (originalRows < 1 || originalCols < 1)
        throw std::invalid_argument("seamsInOriginalCoordinates: size must be at least 1x1");
    // Two "images" holding each surviving pixel's original row and column.
    // Removing the same seams from them as from the picture keeps them in step.
    EnergyMap rowOf, colOf;
    rowOf.rows = colOf.rows = originalRows;
    rowOf.cols = colOf.cols = originalCols;
    rowOf.values.resize(static_cast<std::size_t>(originalRows) * static_cast<std::size_t>(originalCols));
    colOf.values.resize(rowOf.values.size());
    for (int r = 0; r < originalRows; ++r)
        for (int c = 0; c < originalCols; ++c) {
            rowOf.at(r, c) = r;
            colOf.at(r, c) = c;
        }

    std::vector<std::vector<std::pair<int, int>>> positions;
    positions.reserve(seams.size());
    for (const Seam& seam : seams) {
        std::vector<std::pair<int, int>> original;
        original.reserve(seam.indices.size());
        validateSeam(seam, rowOf.rows, rowOf.cols);
        for (const auto& [r, c] : seamPixels(seam))
            original.emplace_back(static_cast<int>(rowOf.at(r, c)), static_cast<int>(colOf.at(r, c)));
        positions.push_back(std::move(original));
        rowOf = removeSeam(rowOf, seam);
        colOf = removeSeam(colOf, seam);
    }
    return positions;
}
