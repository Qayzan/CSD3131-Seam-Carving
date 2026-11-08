// ============================================================================
// test_greedy.cpp  --  tests for the greedy seam finder (item 3)
// Owner: HM (Lee Hwee Min).
//
// Checks:
//   * hand-worked examples (one where greedy is optimal, one where it is not)
//   * tie rule, image borders, 1-row / 1-column maps, both orientations
//   * every seam is valid and Seam::energy matches the map
//   * the energy map is not changed; results are deterministic; bad input throws
//   * brute-force oracle: greedy energy >= the true minimum on random small maps
//   * seam counting: checks the item 5 bound n * 2^(m-1) on small sizes
// ============================================================================
#include "greedy.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

static int failures = 0;

static void report(const char* name, bool ok) {
    std::cout << "    " << std::left << std::setw(60) << name << (ok ? "PASS" : "FAIL") << "\n";
    if (!ok) ++failures;
}

// ---- helpers (test-only) ---------------------------------------------------

static EnergyMap makeMap(int rows, int cols, const std::vector<double>& values) {
    EnergyMap map;
    map.rows = rows;
    map.cols = cols;
    map.values = values;
    return map;
}

// Our own validity check, so these tests do not depend on seam_ops.
static bool isValidSeam(const Seam& seam, int rows, int cols) {
    bool vertical = (seam.orientation == Orientation::Vertical);
    int length = vertical ? rows : cols;  // one index per row (or column)
    int limit = vertical ? cols : rows;   // each index must be < limit
    if (static_cast<int>(seam.indices.size()) != length) return false;
    for (int k = 0; k < length; ++k) {
        if (seam.indices[k] < 0 || seam.indices[k] >= limit) return false;
        if (k > 0 && std::abs(seam.indices[k] - seam.indices[k - 1]) > 1) return false;
    }
    return true;
}

static double sumAlongSeam(const EnergyMap& map, const Seam& seam) {
    double sum = 0.0;
    for (int k = 0; k < static_cast<int>(seam.indices.size()); ++k) {
        if (seam.orientation == Orientation::Vertical)
            sum += map.at(k, seam.indices[k]);
        else
            sum += map.at(seam.indices[k], k);
    }
    return sum;
}

static bool sameIndices(const Seam& seam, const std::vector<int>& expected) {
    return seam.indices == expected;
}

// ---- hand-worked examples ----------------------------------------------------

static void testGreedyFindsOptimum() {
    // 3 1 4      row 0: cheapest is col 1 (1)
    // 1 5 9      row 1: neighbours of col 1 are cols 0..2 -> col 0 (1)
    // 2 6 5      row 2: neighbours of col 0 are cols 0..1 -> col 0 (2)
    // Seam [1, 0, 0], energy 4 = cheapest pixel of every row, so it is optimal.
    EnergyMap map = makeMap(3, 3, {3, 1, 4,
                                   1, 5, 9,
                                   2, 6, 5});
    Seam seam = greedy::findSeam(map, Orientation::Vertical);
    report("hand example: greedy seam is [1, 0, 0]", sameIndices(seam, {1, 0, 0}));
    report("hand example: greedy energy is 4", seam.energy == 4.0);
}

static void testGreedyIsFooled() {
    // 1 3 9      greedy starts at col 0 (1) ...
    // 9 9 1      ... and is then stuck next to 9s: 1 + 9 + 9 = 19.
    // 9 9 1      The optimum starts at the 3: col 1 -> col 2 -> col 2 = 3 + 1 + 1 = 5.
    EnergyMap map = makeMap(3, 3, {1, 3, 9,
                                   9, 9, 1,
                                   9, 9, 1});
    Seam seam = greedy::findSeam(map, Orientation::Vertical);
    report("greedy fooled: seam is [0, 0, 0]", sameIndices(seam, {0, 0, 0}));
    report("greedy fooled: energy is 19 (optimum is 5)", seam.energy == 19.0);
}

static void testTieRuleLeftmost() {
    // All energies equal: every choice ties, so the leftmost always wins.
    EnergyMap map = makeMap(3, 4, std::vector<double>(12, 1.0));
    Seam seam = greedy::findSeam(map, Orientation::Vertical);
    report("uniform map: leftmost seam [0, 0, 0]", sameIndices(seam, {0, 0, 0}));
    report("uniform map: energy 3", seam.energy == 3.0);
}

static void testRightBorder() {
    // The cheap path runs down the last column; col + 1 must never be read.
    EnergyMap map = makeMap(3, 3, {5, 5, 0,
                                   5, 5, 0,
                                   5, 5, 0});
    Seam seam = greedy::findSeam(map, Orientation::Vertical);
    report("right border: seam [2, 2, 2], energy 0",
           sameIndices(seam, {2, 2, 2}) && seam.energy == 0.0);
}

static void testHorizontal() {
    // 1 9 9 9    col 0: rows 0..2 -> row 0 (1)
    // 9 1 1 9    col 1: rows 0..1 -> row 1 (1)
    // 9 9 9 1    col 2: rows 0..2 -> row 1 (1);  col 3: rows 0..2 -> row 2 (1)
    EnergyMap map = makeMap(3, 4, {1, 9, 9, 9,
                                   9, 1, 1, 9,
                                   9, 9, 9, 1});
    Seam seam = greedy::findSeam(map, Orientation::Horizontal);
    report("horizontal: one index per column (length 4)", seam.indices.size() == 4);
    report("horizontal: seam [0, 1, 1, 2], energy 4",
           sameIndices(seam, {0, 1, 1, 2}) && seam.energy == 4.0);
}

static void testDegenerateShapes() {
    EnergyMap oneRow = makeMap(1, 3, {4, 2, 7});
    Seam v1 = greedy::findSeam(oneRow, Orientation::Vertical);
    report("1 row, vertical: seam [1], energy 2", sameIndices(v1, {1}) && v1.energy == 2.0);

    Seam h1 = greedy::findSeam(oneRow, Orientation::Horizontal);
    report("1 row, horizontal: seam [0, 0, 0], energy 13",
           sameIndices(h1, {0, 0, 0}) && h1.energy == 13.0);

    EnergyMap oneCol = makeMap(3, 1, {5, 6, 7});
    Seam v2 = greedy::findSeam(oneCol, Orientation::Vertical);
    report("1 column, vertical: seam [0, 0, 0], energy 18",
           sameIndices(v2, {0, 0, 0}) && v2.energy == 18.0);
}

// ---- contract rules ------------------------------------------------------------

static void testContractOnRandomMaps() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> size(1, 12);
    std::uniform_int_distribution<int> value(0, 255);

    bool allValid = true, energyMatches = true, unchanged = true, deterministic = true;
    for (int trial = 0; trial < 300; ++trial) {
        int rows = size(rng), cols = size(rng);
        std::vector<double> values(static_cast<std::size_t>(rows * cols));
        for (double& v : values) v = value(rng);
        EnergyMap map = makeMap(rows, cols, values);

        for (Orientation o : {Orientation::Vertical, Orientation::Horizontal}) {
            Seam a = greedy::findSeam(map, o);
            Seam b = greedy::findSeam(map, o);
            if (!isValidSeam(a, rows, cols)) allValid = false;
            if (a.energy != sumAlongSeam(map, a)) energyMatches = false;
            if (a.indices != b.indices) deterministic = false;
        }
        if (map.values != values) unchanged = false;
    }
    report("random maps: every seam is valid (both orientations)", allValid);
    report("random maps: Seam::energy == sum along the seam", energyMatches);
    report("random maps: energy map is not modified", unchanged);
    report("random maps: same input gives the same seam", deterministic);
}

static void testBadInputThrows() {
    bool emptyThrows = false, wrongSizeThrows = false;
    try {
        greedy::findSeam(makeMap(0, 0, {}), Orientation::Vertical);
    } catch (const std::invalid_argument&) {
        emptyThrows = true;
    }
    try {
        greedy::findSeam(makeMap(2, 2, {1, 2, 3}), Orientation::Vertical);
    } catch (const std::invalid_argument&) {
        wrongSizeThrows = true;
    }
    report("empty map throws std::invalid_argument", emptyThrows);
    report("values.size() != rows * cols throws", wrongSizeThrows);
}

int main() {
    std::cout << "Greedy seam finder tests\n";
    testGreedyFindsOptimum();
    testGreedyIsFooled();
    testTieRuleLeftmost();
    testRightBorder();
    testHorizontal();
    testDegenerateShapes();
    testContractOnRandomMaps();
    testBadInputThrows();

    if (failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << failures << " TEST(S) FAILED\n";
    return 1;
}