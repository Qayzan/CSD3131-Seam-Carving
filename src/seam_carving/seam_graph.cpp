// ============================================================================
// seam_graph.cpp  --  graph construction for the graph cut seam (see seam_graph.hpp)
// Owner: syazwani (Graph Cut part 1).
// ============================================================================
#include "seam_graph.hpp"

#include <climits>
#include <cmath>
#include <stdexcept>

namespace graphcut {

ll toCapacity(double energy) {
    if (energy < 0.0) energy = 0.0;
    return (ll)std::llround(energy * kEnergyScale);
}

SeamGraph buildVerticalSeamGraph(const Grid& energy) {
    if (energy.empty() || energy[0].empty())
        throw std::invalid_argument("energy map must be at least 1x1");
    int m = (int)energy.size();      // rows
    int n = (int)energy[0].size();   // columns
    for (int i = 1; i < m; ++i)
        if ((int)energy[i].size() != n)
            throw std::invalid_argument("all rows of the energy map must have the same length");
    // Node and edge ids are ints: about 8 edge entries per pixel (4 edges + reverses).
    if ((ll)m * n > INT_MAX / 16)
        throw std::invalid_argument("energy map too large for the graph cut");

    SeamGraph g{FlowNetwork(m * n + 2), m, n, m * n, m * n + 1};

    for (int i = 0; i < m; ++i) {
        g.net.addEdge(g.source, g.node(i, 0), INF);             // 1. source edge

        for (int j = 0; j < n; ++j) {
            int here = g.node(i, j);
            ll cost = toCapacity(energy[i][j]);

            if (j < n - 1) {
                g.net.addEdge(here, here + 1, cost);           // 2. forward: pay to remove (i,j)
                g.net.addEdge(here + 1, here, INF);            // 3. backward: keep S one block
            } else {
                g.net.addEdge(here, g.sink, cost);             // 2. last column goes to the sink
            }

            if (j >= 1) {                                      // 4. diagonals (need a column j-1)
                if (i > 0)     g.net.addEdge(here, g.node(i - 1, j - 1), INF);
                if (i < m - 1) g.net.addEdge(here, g.node(i + 1, j - 1), INF);
            }
        }
    }
    return g;
}

}  // namespace graphcut
