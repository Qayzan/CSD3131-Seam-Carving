// ============================================================================
// test_seam_extraction.cpp -- Tests for Graph Cut seam extraction
// Owner: Kervin (Graph Cut Part 2).
//
// Tests seam validity and compares Graph Cut's minimum seam cost
// against a reference Dynamic Programming implementation.
// ============================================================================

#include "seam_extraction.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>

using graphcut::Grid;

static long long dpCost(const Grid& energy) {
    int rows = (int)energy.size(), cols = (int)energy[0].size();
    std::vector<long long> previous(cols), current(cols);
    for (int c = 0; c < cols; ++c) previous[c] = graphcut::toCapacity(energy[0][c]);
    for (int r = 1; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            long long best = previous[c];
            if (c > 0) best = std::min(best, previous[c - 1]);
            if (c + 1 < cols) best = std::min(best, previous[c + 1]);
            current[c] = best + graphcut::toCapacity(energy[r][c]);
        }
        previous.swap(current);
    }
    return *std::min_element(previous.begin(), previous.end());
}

static void check(const Grid& energy) {
    auto graph = graphcut::buildVerticalSeamGraph(energy);
    long long cutCost = graph.net.maxFlow(graph.source, graph.sink);
    auto seam = graphcut::extractVerticalSeam(graph);
    assert(seam.size() == energy.size());
    long long seamCost = 0;
    for (int r = 0; r < (int)seam.size(); ++r) {
        assert(seam[r] >= 0 && seam[r] < (int)energy[0].size());
        if (r) assert(std::abs(seam[r] - seam[r - 1]) <= 1);
        seamCost += graphcut::toCapacity(energy[r][seam[r]]);
    }
    assert(seamCost == cutCost);
    assert(seamCost == dpCost(energy));
    std::cout << "PASS " << energy.size() << "x" << energy[0].size() << " seam=";
    for (int x : seam) std::cout << x << " ";
    std::cout << "cost=" << seamCost << '\n';
}

int main() {
    check({{1, 8, 8}, {1, 8, 8}, {1, 8, 8}});
    check({{8, 8, 1}, {8, 1, 8}, {1, 8, 8}});
    check({{4}});
    check({{7, 0, 4, 2}});
    check({{2}, {5}, {1}});
    check({{2, 1, 5}, {4, 1, 7}, {9, 1, 2}});
}
