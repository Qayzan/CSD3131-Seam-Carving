// ============================================================================
// test_integration.cpp  --  every seam finder in this build, through the
// shared foundation (item 1, FF)
//
// Teammates' finders are detected by their headers. When a finder is merged it
// is tested here automatically; until then its checks print SKIP. Checks per
// finder:
//   * it follows the contract on random maps (carve() validates every seam);
//   * DP and graph cut: energy equals the exhaustive optimum on tiny maps;
//   * greedy: energy is never below the optimum;
//   * full resizes of real pictures (energy + finder + removal) reach the target.
// The exhaustive oracle itself is checked first.
// ============================================================================
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "carver.hpp"
#include "check.hpp"
#include "energy.hpp"
#include "image_utils.hpp"
#include "seam_ops.hpp"
#include "test_helpers.hpp"

#if __has_include("dynamic_programming.hpp")
#include "dynamic_programming.hpp"
#define HAVE_DP 1
#endif
#if __has_include("greedy.hpp")
#include "greedy.hpp"
#define HAVE_GREEDY 1
#endif
#if __has_include("graph_cut.hpp")
#include "graph_cut.hpp"
#define HAVE_GRAPH_CUT 1
#elif __has_include("seam_extraction.hpp")
#include "seam_extraction.hpp"
#define HAVE_GRAPH_CUT_EXTRACTION 1
#endif

using testing_helpers::bruteForceSeam;
using testing_helpers::makeMap;
using testing_helpers::randomImage;

namespace {

const Orientation V = Orientation::Vertical;
const Orientation H = Orientation::Horizontal;

#ifdef HAVE_GRAPH_CUT_EXTRACTION
// Same temporary bridge as in src/main.cpp, until graphcut::findSeam(EnergyMap, Orientation) exists.
Seam graphCutViaExtraction(const EnergyMap& energy, Orientation orientation) {
    const EnergyMap map = orientation == V ? energy : transpose(energy);
    graphcut::Grid grid(static_cast<std::size_t>(map.rows), std::vector<double>(static_cast<std::size_t>(map.cols)));
    for (int r = 0; r < map.rows; ++r)
        for (int c = 0; c < map.cols; ++c) grid[r][c] = map.at(r, c);
    Seam seam;
    seam.orientation = orientation;
    seam.indices = graphcut::findVerticalGraphCutSeam(grid);
    seam.energy = seamEnergy(energy, seam);
    return seam;
}
#endif

struct Finder {
    SeamFinder find;
    bool optimal;       // must match the exhaustive minimum
    double tolerance;   // allowed difference from it (graph cut rounds energies to 1/1000)
};

std::map<std::string, Finder> finders() {
    std::map<std::string, Finder> all;
#ifdef HAVE_DP
    all["dp"] = {dp::findSeam, true, 1e-9};
#endif
#ifdef HAVE_GREEDY
    all["greedy"] = {greedy::findSeam, false, 0.0};
#endif
#ifdef HAVE_GRAPH_CUT
    all["graphcut"] = {graphcut::findSeam, true, 1e-2};
#elif defined(HAVE_GRAPH_CUT_EXTRACTION)
    all["graphcut"] = {graphCutViaExtraction, true, 1e-2};
#endif
    return all;
}

void testOracle() {
    const EnergyMap worked = makeMap(4, 5, {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 1, 9, 7, 2, 6, 5, 3, 5});
    CHECK(testing_helpers::countVerticalSeams(4, 5) == 95);
    const Seam best = bruteForceSeam(worked, V);
    CHECK(best.indices == std::vector<int>({1, 1, 2, 3}) && best.energy == 7.0);
    CHECK(bruteForceSeam(makeMap(1, 4, {4, 2, 2, 7}), V).indices == std::vector<int>({1}));  // leftmost tie
    const Seam horizontal = bruteForceSeam(worked, H);
    CHECK(horizontal.indices.size() == 5 && isValidSeam(horizontal, 4, 5));
    CHECK(horizontal.energy == seamEnergy(worked, horizontal));
}

void testFindersAgainstOracle(const std::string& name, const Finder& f) {
    std::mt19937 rng(31);
    std::uniform_int_distribution<int> size(1, 6), value(0, 9);
    for (int trial = 0; trial < 60; ++trial) {
        const int rows = size(rng), cols = size(rng);
        EnergyMap map = makeMap(rows, cols, {});
        for (int k = 0; k < rows * cols; ++k) map.values.push_back(value(rng));
        for (Orientation o : {V, H}) {
            const Seam s = f.find(map, o);
            const Seam best = bruteForceSeam(map, o);
            CHECK(s.orientation == o && isValidSeam(s, rows, cols));
            CHECK(std::fabs(s.energy - seamEnergy(map, s)) <= 1e-9);
            if (f.optimal)
                CHECK(std::fabs(s.energy - best.energy) <= f.tolerance * rows * cols + 1e-12);
            else
                CHECK(s.energy >= best.energy - 1e-12);
        }
    }
    (void)name;
}

void testFullResize(const std::string& name, const Finder& f) {
    std::mt19937 rng(32);
    // Graph cut is far slower (Edmonds-Karp), so it gets a smaller picture.
    const bool slow = name == "graphcut";
    const Image image = randomImage(rng, slow ? 12 : 40, slow ? 14 : 50, 3);
    const int targetRows = slow ? 10 : 30, targetCols = slow ? 11 : 35;
    for (CarveOrder order : {CarveOrder::WidthFirst, CarveOrder::Alternate}) {
        const CarveResult r = carve(image, targetRows, targetCols, f.find, order);
        CHECK(r.image.rows == targetRows && r.image.cols == targetCols && r.image.channels == 3);
        CHECK(r.seams.size() == static_cast<std::size_t>((image.rows - targetRows) + (image.cols - targetCols)));
    }
}

}  // namespace

int main() {
    check::run("exhaustive oracle (95 seams, worked example)", testOracle);
    const auto all = finders();
    const char* names[] = {"dp", "greedy", "graphcut"};
    for (const char* name : names) {
        const auto it = all.find(name);
        if (it == all.end()) {
            std::cout << "SKIP  " << name << ": not in this build yet (no header found)\n";
            continue;
        }
        check::run((std::string(name) + ": contract + comparison with the exhaustive optimum").c_str(),
                   [&] { testFindersAgainstOracle(name, it->second); });
        check::run((std::string(name) + ": full resize through carve()").c_str(),
                   [&] { testFullResize(name, it->second); });
    }
    return check::summary("test_integration");
}
