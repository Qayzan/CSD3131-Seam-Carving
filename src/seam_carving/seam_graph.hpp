// ============================================================================
// seam_graph.hpp  --  build the flow network whose minimum s-t cut is the
//                     minimum-energy vertical seam
// Owner: syazwani (Graph Cut part 1: graph construction + max-flow).
//
// One node per pixel, plus a source s and a sink t. The seam splits every row
// into a LEFT part (source side S) and a RIGHT part (sink side T). In each row
// the LAST pixel of the left part is the pixel to remove.
//
//   s --INF--> (i,0) --e[i][0]--> (i,1) --e[i][1]--> ... (i,n-1) --e[i][n-1]--> t
//
//  1. Source edge s -> (i,0), INF: column 0 is always on the left.
//  2. Forward edge (i,j) -> (i,j+1), capacity e[i][j]: cutting it means
//     "remove pixel (i,j)" and pays its energy. The last column's forward
//     edge goes to t.
//  3. Backward edge (i,j+1) -> (i,j), INF: the left part of every row is one
//     block, so each row is cut exactly once.
//  4. Diagonal edges (i,j) -> (i-1,j-1) and (i,j) -> (i+1,j-1), INF: seam
//     positions in neighbouring rows differ by at most 1 column.
//
// NOTE (small change from the brief): the brief makes the last column -> t
// edge INF, so the last column could never be removed. Here it has capacity
// e[i][n-1], so every pixel can be on the seam, exactly as in the DP version.
//
// Max-flow min-cut theorem: max-flow value = capacity of the cheapest cut =
// (scaled) energy of the minimum seam.
//
// Handover to KO (Graph Cut part 2): after graph.net.maxFlow(graph.source,
// graph.sink), the residual capacities are in graph.net.edges(). The source
// side S is every node reachable from graph.source through edges with cap > 0;
// in row i the seam pixel is the rightmost column j with node(i, j) in S.
// Horizontal seams: build the graph from the transposed energy map.
// ============================================================================
#ifndef SEAM_GRAPH_HPP
#define SEAM_GRAPH_HPP

#include <vector>

#include "max_flow.hpp"

namespace graphcut {

// energy[row][col]. Replace with the team's energy-map type once the shared
// foundation (FF) is written; only buildVerticalSeamGraph reads it.
typedef std::vector<std::vector<double> > Grid;

// Max-flow works with whole numbers, so energies are scaled and rounded.
const double kEnergyScale = 1000.0;

// "Infinite" capacity: far bigger than any real cut, but small enough that
// adding flow to it never overflows a long long.
const ll INF = 1LL << 50;

// Energy -> integer capacity (negative energies count as 0).
ll toCapacity(double energy);

struct SeamGraph {
    FlowNetwork net;
    int rows;
    int cols;
    int source;  // = rows * cols
    int sink;    // = rows * cols + 1

    int node(int r, int c) const { return r * cols + c; }  // pixel -> node id, row by row
};

// Builds the graph described above for a vertical seam.
// Throws std::invalid_argument if energy is empty or its rows differ in length.
SeamGraph buildVerticalSeamGraph(const Grid& energy);

}  // namespace graphcut

#endif  // SEAM_GRAPH_HPP
