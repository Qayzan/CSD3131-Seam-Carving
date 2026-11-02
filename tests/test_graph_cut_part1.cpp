// ============================================================================
// test_graph_cut_part1.cpp  --  tests for Graph Cut part 1 (syazwani):
//                               graph construction + Edmonds-Karp max-flow
//
// Part 1 ends at the max-flow value, so these tests check:
//   * Edmonds-Karp on a textbook network with a known answer
//   * the graph has exactly the edges and capacities of the design
//   * max-flow == minimum seam energy (checked against a DP oracle)
//   * after max-flow no augmenting path is left (what KO's extraction relies on)
//
// Build and run (Visual Studio Developer PowerShell, from the repository root):
//     cl /std:c++17 /EHsc /O2 /W4 /I src\seam_carving tests\test_graph_cut_part1.cpp src\seam_carving\max_flow.cpp src\seam_carving\seam_graph.cpp
//     .\test_graph_cut_part1.exe
// Build (g++):
//     g++ -O2 -std=c++17 -Wall -Wextra -I src/seam_carving -o test_graph_cut_part1 tests/test_graph_cut_part1.cpp src/seam_carving/max_flow.cpp src/seam_carving/seam_graph.cpp
//     ./test_graph_cut_part1
// ============================================================================
#include "max_flow.hpp"
#include "seam_graph.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

using namespace graphcut;

static int failures = 0;

static void report(const char* name, bool ok) {
    std::cout << "    " << std::left << std::setw(58) << name << (ok ? "PASS" : "FAIL") << "\n";
    if (!ok) ++failures;
}

static Grid makeGrid(std::initializer_list<std::initializer_list<double> > rows) {
    Grid g;
    for (const auto& r : rows) g.push_back(std::vector<double>(r));
    return g;
}

static Grid randomGrid(int m, int n, int maxEnergy, std::mt19937& rng) {
    std::uniform_int_distribution<int> pick(0, maxEnergy);
    Grid g(m, std::vector<double>(n));
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j) g[i][j] = pick(rng);
    return g;
}

// The Dynamic Programming answer: minimum total (scaled) cost of any vertical
// seam. Test-only oracle; the real DP is YN's.
static ll dpMinVerticalSeamCost(const Grid& energy) {
    int m = (int)energy.size();
    int n = (int)energy[0].size();
    std::vector<ll> prev(n), cur(n);
    for (int j = 0; j < n; ++j) prev[j] = toCapacity(energy[0][j]);
    for (int i = 1; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            ll up = prev[j];
            if (j > 0)     up = std::min(up, prev[j - 1]);
            if (j < n - 1) up = std::min(up, prev[j + 1]);
            cur[j] = toCapacity(energy[i][j]) + up;
        }
        prev.swap(cur);
    }
    return *std::min_element(prev.begin(), prev.end());
}

// Capacity of the REAL edge u -> v (even id), or -1 if there is no such edge.
static ll realEdgeCap(const FlowNetwork& net, int u, int v) {
    for (int id : net.edgesFrom(u))
        if (id % 2 == 0 && net.edges()[id].to == v) return net.edges()[id].cap;
    return -1;
}

// Is t reachable from s through edges with remaining capacity?
static bool residualPathExists(const FlowNetwork& net, int s, int t) {
    std::vector<char> seen(net.nodeCount(), 0);
    std::vector<int> queue(1, s);
    seen[s] = 1;
    for (size_t head = 0; head < queue.size(); ++head) {
        for (int id : net.edgesFrom(queue[head])) {
            const Edge& e = net.edges()[id];
            if (e.cap > 0 && !seen[e.to]) {
                seen[e.to] = 1;
                queue.push_back(e.to);
            }
        }
    }
    return seen[t] != 0;
}

int main() {
    // ---- Test 1: Edmonds-Karp on a textbook network --------------------------
    // The 6-node example from CLRS (Cormen et al.), Figure 26.1: max-flow = 23.
    std::cout << "Test 1: Edmonds-Karp on known networks\n";
    {
        FlowNetwork net(6);
        net.addEdge(0, 1, 16); net.addEdge(0, 2, 13);
        net.addEdge(1, 3, 12); net.addEdge(2, 1, 4);
        net.addEdge(2, 4, 14); net.addEdge(3, 2, 9);
        net.addEdge(3, 5, 20); net.addEdge(4, 3, 7);
        net.addEdge(4, 5, 4);
        report("CLRS Figure 26.1 network: max-flow = 23", net.maxFlow(0, 5) == 23);
        report("CLRS network: no augmenting path left afterwards", !residualPathExists(net, 0, 5));

        FlowNetwork none(3);  // sink not connected at all
        none.addEdge(0, 1, 5);
        report("sink unreachable: max-flow = 0", none.maxFlow(0, 2) == 0);

        // Two separate paths 0-1-3 and 0-2-3 plus a cross edge 1 -> 2:
        // the answer is 2, and the cross edge must not add extra flow.
        FlowNetwork cross(4);
        cross.addEdge(0, 1, 1); cross.addEdge(0, 2, 1);
        cross.addEdge(1, 2, 1); cross.addEdge(1, 3, 1);
        cross.addEdge(2, 3, 1);
        report("diamond network: max-flow = 2", cross.maxFlow(0, 3) == 2);
    }
    std::cout << "\n";

    // ---- Test 2: the seam graph has exactly the designed edges ---------------
    std::cout << "Test 2: graph construction on the 3x3 lesson grid [5 1 7 / 4 2 6 / 8 1 3]\n";
    {
        Grid e = makeGrid({{5, 1, 7}, {4, 2, 6}, {8, 1, 3}});
        SeamGraph g = buildVerticalSeamGraph(e);
        const FlowNetwork& net = g.net;

        report("node ids: 9 pixels + s (9) + t (10)",
               net.nodeCount() == 11 && g.source == 9 && g.sink == 10 && g.node(2, 1) == 7);
        // 3 source + 9 forward + 6 backward + 8 diagonal = 26 edges, each with a reverse.
        report("26 edges (52 entries with their reverse edges)", net.edges().size() == 52);
        report("1. source edges s -> (i,0) are INF",
               realEdgeCap(net, 9, 0) == INF && realEdgeCap(net, 9, 3) == INF &&
               realEdgeCap(net, 9, 6) == INF);
        report("2. forward (0,0) -> (0,1) = 5000, (2,1) -> (2,2) = 1000",
               realEdgeCap(net, 0, 1) == 5000 && realEdgeCap(net, 7, 8) == 1000);
        report("2. last column -> t = its energy (7000, 6000, 3000)",
               realEdgeCap(net, 2, 10) == 7000 && realEdgeCap(net, 5, 10) == 6000 &&
               realEdgeCap(net, 8, 10) == 3000);
        report("3. backward (0,1) -> (0,0) is INF", realEdgeCap(net, 1, 0) == INF);
        report("4. diagonals (1,1) -> (0,0) and (1,1) -> (2,0) are INF",
               realEdgeCap(net, 4, 0) == INF && realEdgeCap(net, 4, 6) == INF);
        report("4. no diagonal leaves column 0, no edge goes up or down",
               realEdgeCap(net, 3, 1) == -1 && realEdgeCap(net, 1, 4) == -1);
        report("no edge into s, no edge out of t",
               realEdgeCap(net, 0, 9) == -1 && net.edgesFrom(10).size() == 3);
    }
    {
        // Edge count formula for an m x n grid:
        //   m (source) + m*n (forward) + m*(n-1) (backward) + 2*(m-1)*(n-1) (diagonal)
        bool ok = true;
        int sizes[][2] = {{1, 1}, {1, 5}, {5, 1}, {2, 2}, {4, 7}, {7, 4}, {10, 10}};
        for (auto& sz : sizes) {
            int m = sz[0], n = sz[1];
            SeamGraph g = buildVerticalSeamGraph(Grid(m, std::vector<double>(n, 1.0)));
            size_t expected = (size_t)(m + m * n + m * (n - 1) + 2 * (m - 1) * (n - 1));
            if (g.net.edges().size() != 2 * expected) ok = false;
        }
        report("edge count = m + mn + m(n-1) + 2(m-1)(n-1) on 7 shapes", ok);
    }
    {
        bool threw = 0;
        try { buildVerticalSeamGraph(Grid()); } catch (const std::invalid_argument&) { threw = 1; }
        bool threw2 = 0;
        try { buildVerticalSeamGraph(makeGrid({{1, 2}, {3}})); } catch (const std::invalid_argument&) { threw2 = 1; }
        report("empty or ragged energy map throws invalid_argument", threw && threw2);
    }
    std::cout << "\n";

    // ---- Test 3: max-flow == minimum seam energy ------------------------------
    std::cout << "Test 3: max-flow == minimum seam energy\n";
    {
        SeamGraph g = buildVerticalSeamGraph(makeGrid({{5, 1, 7}, {4, 2, 6}, {8, 1, 3}}));
        report("3x3 lesson grid: max-flow = 4000 (seam 1 + 2 + 1)",
               g.net.maxFlow(g.source, g.sink) == 4000);

        SeamGraph h = buildVerticalSeamGraph(makeGrid({{9, 9, 1, 9, 9},
                                                       {9, 1, 9, 9, 9},
                                                       {9, 1, 9, 9, 9},
                                                       {9, 9, 1, 9, 9},
                                                       {9, 9, 9, 1, 9}}));
        report("5x5 grid of 9s with a path of 1s: max-flow = 5000",
               h.net.maxFlow(h.source, h.sink) == 5000);

        SeamGraph last = buildVerticalSeamGraph(makeGrid({{9, 9, 1}, {9, 9, 1}, {9, 1, 9}}));
        report("cheapest seam uses the last column: max-flow = 3000",
               last.net.maxFlow(last.source, last.sink) == 3000);
    }
    {
        struct Case { int m, n, trials, maxE; };
        Case cases[] = {
            {  1,  1, 20, 20 }, {  1,  6, 50, 20 }, {  6,  1, 50, 20 },
            {  2,  2, 200, 20 }, {  3,  7, 200, 20 }, {  8,  8, 200, 20 },
            { 15, 10, 100, 20 }, { 10, 15, 100, 20 }, {  8,  8, 100, 1000 },
            { 30, 30, 30, 255 }, { 40, 25, 10, 255 },
        };
        std::mt19937 rng(12345);  // fixed seed: same grids every run
        for (const Case& c : cases) {
            int bad = 0;
            for (int trial = 0; trial < c.trials; ++trial) {
                Grid e = randomGrid(c.m, c.n, c.maxE, rng);
                SeamGraph g = buildVerticalSeamGraph(e);
                ll flow = g.net.maxFlow(g.source, g.sink);
                if (flow != dpMinVerticalSeamCost(e) || residualPathExists(g.net, g.source, g.sink))
                    ++bad;
            }
            char name[80];
            std::snprintf(name, sizeof(name), "%2d x %2d  (%3d random grids, energy 0..%d)",
                          c.m, c.n, c.trials, c.maxE);
            report(name, bad == 0);
        }
    }
    std::cout << "\n";

    // ---- Test 4: how long do graph construction + max-flow take? -------------
    std::cout << "Test 4: time for one seam's graph + max-flow (random energies 0..255)\n";
    {
        std::mt19937 rng(99);
        int sizes[] = { 25, 50, 75, 100 };
        for (int size : sizes) {
            Grid e = randomGrid(size, size, 255, rng);
            auto t0 = std::chrono::steady_clock::now();
            SeamGraph g = buildVerticalSeamGraph(e);
            auto t1 = std::chrono::steady_clock::now();
            g.net.maxFlow(g.source, g.sink);
            auto t2 = std::chrono::steady_clock::now();
            std::cout << "    " << std::right << std::setw(3) << size << " x " << std::setw(3) << size
                      << "  : build " << std::fixed << std::setprecision(1)
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms, max-flow "
                      << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms\n";
        }
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED")
              << " (failures: " << failures << ")\n";
    return failures == 0 ? 0 : 1;
}
