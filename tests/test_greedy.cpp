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

// ---- brute-force oracle ------------------------------------------------------------

// Tries every vertical seam that continues from pixel (row, col). Exponential,
// so only for tiny maps -- that is exactly what item 5 proves.
static void searchAllSeams(const EnergyMap& map, int row, int col, double sumSoFar,
                           double& best) {
    double sum = sumSoFar + map.at(row, col);
    if (row == map.rows - 1) {
        best = std::min(best, sum);
        return;
    }
    for (int next = col - 1; next <= col + 1; ++next) {
        if (next >= 0 && next < map.cols) searchAllSeams(map, row + 1, next, sum, best);
    }
}

static double bruteForceMinVertical(const EnergyMap& map) {
    double best = std::numeric_limits<double>::infinity();
    for (int col = 0; col < map.cols; ++col) searchAllSeams(map, 0, col, 0.0, best);
    return best;
}

static EnergyMap transposeMap(const EnergyMap& map) {
    EnergyMap t = makeMap(map.cols, map.rows,
                          std::vector<double>(static_cast<std::size_t>(map.rows * map.cols)));
    for (int r = 0; r < map.rows; ++r)
        for (int c = 0; c < map.cols; ++c) t.values[c * map.rows + r] = map.at(r, c);
    return t;
}

static void testAgainstBruteForce() {
    std::mt19937 rng(2026);
    std::uniform_int_distribution<int> size(1, 6);
    std::uniform_int_distribution<int> value(0, 9);

    int trials = 500, optimalCount = 0;
    bool neverBelowOptimum = true;
    for (int trial = 0; trial < trials; ++trial) {
        int rows = size(rng), cols = size(rng);
        std::vector<double> values(static_cast<std::size_t>(rows * cols));
        for (double& v : values) v = value(rng);
        EnergyMap map = makeMap(rows, cols, values);

        double optimumV = bruteForceMinVertical(map);
        double optimumH = bruteForceMinVertical(transposeMap(map));  // horizontal = vertical of the transpose
        Seam v = greedy::findSeam(map, Orientation::Vertical);
        Seam h = greedy::findSeam(map, Orientation::Horizontal);

        if (v.energy < optimumV || h.energy < optimumH) neverBelowOptimum = false;
        if (v.energy == optimumV) ++optimalCount;
    }
    report("oracle: greedy energy >= brute-force minimum (500 maps)", neverBelowOptimum);
    std::cout << "    (greedy was optimal on " << optimalCount << " of " << trials
              << " random vertical cases)\n";
}

// ---- seam counting (item 5) -----------------------------------------------------------

// Number of vertical seams in an m x n image, counted row by row:
// ways[c] = number of seams from the top row that end at column c of the current row.
static unsigned long long countSeams(int m, int n) {
    std::vector<unsigned long long> ways(static_cast<std::size_t>(n), 1), next(ways.size());
    for (int row = 1; row < m; ++row) {
        for (int c = 0; c < n; ++c) {
            next[c] = ways[c];
            if (c > 0) next[c] += ways[c - 1];
            if (c < n - 1) next[c] += ways[c + 1];
        }
        ways.swap(next);
    }
    unsigned long long total = 0;
    for (unsigned long long w : ways) total += w;
    return total;
}

static void testSeamCountBound() {
    // Known value from docs/algorithm_design.md: a 4 x 5 matrix has 95 seams.
    report("4 x 5 image has exactly 95 seams", countSeams(4, 5) == 95);

    // Item 5: for n >= 2,  n * 2^(m-1) <= #seams <= n * 3^(m-1).
    bool boundHolds = true;
    for (int m = 1; m <= 15; ++m) {
        for (int n = 2; n <= 10; ++n) {
            unsigned long long count = countSeams(m, n), lower = n, upper = n;
            for (int k = 1; k < m; ++k) {
                lower *= 2;
                upper *= 3;
            }
            if (count < lower || count > upper) boundHolds = false;
        }
    }
    report("n*2^(m-1) <= #seams <= n*3^(m-1) for m<=15, 2<=n<=10", boundHolds);

    // For n = 2 the lower bound is exact: 2 * 2^(m-1) = 2^m.
    bool tightForTwo = true;
    for (int m = 1; m <= 20; ++m)
        if (countSeams(m, 2) != (1ULL << m)) tightForTwo = false;
    report("n = 2: exactly 2^m seams (lower bound is tight)", tightForTwo);
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
    testAgainstBruteForce();
    testSeamCountBound();

    if (failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << failures << " TEST(S) FAILED\n";
    return 1;
}