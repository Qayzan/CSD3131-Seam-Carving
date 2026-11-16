// ============================================================================
// test_dynamic_programming.cpp  --  DP seam finder (brief Task 1(b), FF)
//
// Checked against independent references:
//   * the worked example of docs/algorithm_design.md 2.5 (tables by hand);
//   * every M[i][j] against a plain recursion over all paths ending at (i, j);
//   * every seam against the exhaustive oracle in test_helpers.hpp, which tries
//     EVERY seam: same energy AND same seam (same tie rule);
//   * the memory-saving version against the full-table version.
// ============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include "carver.hpp"
#include "check.hpp"
#include "dynamic_programming.hpp"
#include "energy.hpp"
#include "image_utils.hpp"
#include "seam_ops.hpp"
#include "test_helpers.hpp"

using testing_helpers::bruteForceSeam;
using testing_helpers::makeMap;
using testing_helpers::randomImage;

namespace {

const Orientation V = Orientation::Vertical;
const Orientation H = Orientation::Horizontal;

EnergyMap worked() {  // docs/algorithm_design.md 2.5
    return makeMap(4, 5, {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 1, 9, 7, 2, 6, 5, 3, 5});
}

EnergyMap randomIntegerMap(std::mt19937& rng, int rows, int cols, int maxValue) {
    std::uniform_int_distribution<int> value(0, maxValue);
    EnergyMap map = makeMap(rows, cols, {});
    for (int k = 0; k < rows * cols; ++k) map.values.push_back(value(rng));
    return map;
}

// Cheapest path from row 0 to (i, j) by trying every path (no table at all).
double slowM(const EnergyMap& e, int i, int j) {
    if (j < 0 || j >= e.cols) return std::numeric_limits<double>::infinity();
    if (i == 0) return e.at(0, j);
    return e.at(i, j) + std::min({slowM(e, i - 1, j - 1), slowM(e, i - 1, j), slowM(e, i - 1, j + 1)});
}

void testWorkedExample() {
    const dp::Table t = dp::cumulativeEnergy(worked());
    const std::vector<double> expectedM = {3, 1, 4, 1, 5, 10, 3, 7, 6, 4, 8, 11, 4, 13, 11, 10, 10, 9, 7, 16};
    CHECK(t.rows == 4 && t.cols == 5);
    CHECK(t.cost == expectedM);
    CHECK(t.step(1, 2) == -1);  // M[0][1] = M[0][3] = 1 tie: the leftmost (column 1) wins
    CHECK(t.step(0, 0) == 0 && t.step(0, 4) == 0);
    const Seam s = dp::backtrack(t);
    CHECK(s.orientation == V);
    CHECK(s.indices == std::vector<int>({1, 1, 2, 3}));
    CHECK(s.energy == 7.0);
    CHECK(dp::findSeam(worked(), V).indices == s.indices);
}

void testTableAgainstRecursion() {
    std::mt19937 rng(41);
    std::uniform_int_distribution<int> size(1, 7);
    for (int trial = 0; trial < 40; ++trial) {
        const EnergyMap e = randomIntegerMap(rng, size(rng), size(rng), 9);
        const dp::Table t = dp::cumulativeEnergy(e);
        for (int i = 0; i < e.rows; ++i)
            for (int j = 0; j < e.cols; ++j) {
                CHECK(t.at(i, j) == slowM(e, i, j));
                // the back-pointer names a neighbour that really gives this minimum
                if (i > 0) {
                    const int k = j + t.step(i, j);
                    CHECK(k >= 0 && k < e.cols && t.at(i, j) == e.at(i, j) + t.at(i - 1, k));
                }
            }
    }
}

void testTieRule() {
    CHECK(dp::findSeam(makeMap(1, 4, {4, 2, 2, 7}), V).indices == std::vector<int>({1}));
    // flat map: every seam costs the same, so the leftmost straight seam wins
    const EnergyMap flat = makeMap(3, 4, std::vector<double>(12, 5.0));
    CHECK(dp::findSeam(flat, V).indices == std::vector<int>({0, 0, 0}));
    CHECK(dp::findSeam(flat, H).indices == std::vector<int>({0, 0, 0, 0}));
    // j+1 must not win a tie against j
    const dp::Table t = dp::cumulativeEnergy(makeMap(2, 3, {5, 1, 1, 0, 0, 0}));
    CHECK(t.step(1, 1) == 0 && t.step(1, 2) == -1 && t.step(1, 0) == +1);
}

void testAgainstExhaustiveOracle() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> size(1, 7);
    int compared = 0;
    for (int trial = 0; trial < 150; ++trial) {
        const int rows = size(rng), cols = size(rng);
        const EnergyMap e = randomIntegerMap(rng, rows, cols, trial % 3 == 0 ? 2 : 9);  // many ties
        for (Orientation o : {V, H}) {
            const Seam s = dp::findSeam(e, o);
            const Seam best = bruteForceSeam(e, o);
            CHECK(s.orientation == o && isValidSeam(s, rows, cols));
            CHECK(s.energy == best.energy);
            CHECK(s.indices == best.indices);
            CHECK(s.energy == seamEnergy(e, s));
            ++compared;
        }
    }
    CHECK(compared == 300);
}

void testHorizontalIsVerticalOfTranspose() {
    std::mt19937 rng(43);
    for (int trial = 0; trial < 20; ++trial) {
        const EnergyMap e = randomIntegerMap(rng, 4 + trial % 5, 9 - trial % 4, 20);  // never square
        const Seam h = dp::findSeam(e, H);
        const Seam v = dp::findSeam(transpose(e), V);
        CHECK(h.orientation == H && h.indices.size() == static_cast<std::size_t>(e.cols));
        CHECK(h.indices == v.indices && h.energy == v.energy);
        CHECK(isValidSeam(h, e.rows, e.cols));
    }
}

void testCompactMatchesFullTable() {
    std::mt19937 rng(44);
    std::uniform_real_distribution<double> value(0.0, 1000.0);
    for (int trial = 0; trial < 30; ++trial) {
        const int rows = 1 + trial * 3, cols = 1 + (trial * 7) % 61;
        EnergyMap e = makeMap(rows, cols, {});
        for (int k = 0; k < rows * cols; ++k) e.values.push_back(value(rng));
        for (Orientation o : {V, H}) {
            const Seam full = dp::findSeam(e, o);
            const Seam compact = dp::findSeamCompact(e, o);
            CHECK(full.indices == compact.indices && full.energy == compact.energy);
            CHECK(isValidSeam(full, rows, cols));
            CHECK(std::fabs(full.energy - seamEnergy(e, full)) <= 1e-9 * std::max(1.0, full.energy));
        }
    }
}

void testSmallShapes() {
    const EnergyMap oneByOne = makeMap(1, 1, {3});
    CHECK(dp::findSeam(oneByOne, V).indices == std::vector<int>({0}));
    CHECK(dp::findSeam(oneByOne, H).indices == std::vector<int>({0}));
    const EnergyMap column = makeMap(4, 1, {1, 2, 3, 4});
    CHECK(dp::findSeam(column, V).indices == std::vector<int>({0, 0, 0, 0}) && dp::findSeam(column, V).energy == 10);
    CHECK(dp::findSeam(column, H).indices == std::vector<int>({0}) && dp::findSeam(column, H).energy == 1);
    const EnergyMap row = makeMap(1, 4, {4, 3, 2, 1});
    CHECK(dp::findSeam(row, V).indices == std::vector<int>({3}));
    CHECK(dp::findSeam(row, H).indices == std::vector<int>({0, 0, 0, 0}) && dp::findSeam(row, H).energy == 10);
}

void testContract() {
    const EnergyMap e = worked();
    const EnergyMap copy = e;
    const Seam a = dp::findSeam(e, H);
    const Seam b = dp::findSeam(e, H);
    CHECK(e.values == copy.values);                          // input untouched
    CHECK(a.indices == b.indices && a.energy == b.energy);   // deterministic
    CHECK_THROWS(std::invalid_argument, dp::findSeam(makeMap(0, 0, {}), V));
    CHECK_THROWS(std::invalid_argument, dp::findSeam(makeMap(2, 2, {1, 2, 3}), V));
    CHECK_THROWS(std::invalid_argument, dp::findSeamCompact(makeMap(0, 3, {}), H));
    CHECK_THROWS(std::invalid_argument, dp::backtrack(dp::Table{}));
}

void testFullResize() {
    // Left 15 columns: random texture (high energy). Right 25 columns: flat
    // grey (energy 0). Every DP seam must go through the flat part, so the
    // texture comes out untouched -- vertically, and horizontally on the
    // transposed picture.
    std::mt19937 rng(45);
    Image image = randomImage(rng, 20, 40, 3);
    for (int r = 0; r < 20; ++r)
        for (int c = 15; c < 40; ++c)
            for (int k = 0; k < 3; ++k) image.at(r, c, k) = 128;
    const CarveResult wide = carve(image, 20, 30, dp::findSeam);
    const CarveResult tall = carve(transpose(image), 30, 20, dp::findSeam);
    CHECK(wide.image.rows == 20 && wide.image.cols == 30 && wide.seams.size() == 10);
    CHECK(tall.image.rows == 30 && tall.image.cols == 20 && tall.seams.size() == 10);
    bool textureKept = true;
    for (int r = 0; r < 20; ++r)
        for (int c = 0; c < 15; ++c)
            for (int k = 0; k < 3; ++k) {
                textureKept = textureKept && wide.image.at(r, c, k) == image.at(r, c, k);
                textureKept = textureKept && tall.image.at(c, r, k) == image.at(r, c, k);
            }
    CHECK(textureKept);
    for (const Seam& s : tall.seams) CHECK(s.orientation == H);

    const Image noise = randomImage(rng, 25, 33, 3);
    const CarveResult n = carve(noise, 20, 25, dp::findSeamCompact, CarveOrder::Alternate);
    CHECK(n.image.rows == 20 && n.image.cols == 25);
}

}  // namespace

int main() {
    check::run("worked example: tables M and back, seam [1, 1, 2, 3]", testWorkedExample);
    check::run("every M[i][j] equals a recursion over all paths", testTableAgainstRecursion);
    check::run("tie rule: leftmost wins", testTieRule);
    check::run("same seam and energy as the exhaustive oracle (300 cases)", testAgainstExhaustiveOracle);
    check::run("horizontal seam = vertical seam of the transpose", testHorizontalIsVerticalOfTranspose);
    check::run("two-row version gives the same seams", testCompactMatchesFullTable);
    check::run("1-pixel-wide and 1-pixel-high maps", testSmallShapes);
    check::run("contract: input untouched, deterministic, bad maps rejected", testContract);
    check::run("full resize through carve()", testFullResize);
    return check::summary("test_dynamic_programming");
}
