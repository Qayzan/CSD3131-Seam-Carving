// ============================================================================
// seam_extraction.cpp -- Graph Cut seam extraction using the residual graph
// Owner: Kervin (Graph Cut Part 2).
//
// After Edmonds-Karp finishes, BFS identifies all nodes reachable from
// the source through edges with remaining capacity.
//
// For each row, the rightmost reachable pixel is selected as the seam.
// The extracted seam is then checked for validity.
// ============================================================================

#include "seam_extraction.hpp"
#include <stdexcept>
#include <vector>
#include <cstdlib>

namespace graphcut {

std::vector<int> extractVerticalSeam(const SeamGraph& graph) {
    const FlowNetwork& net = graph.net;
    std::vector<char> reachable(net.nodeCount(), false);
    std::vector<int> queue;
    queue.push_back(graph.source);
    reachable[graph.source] = true;

    // Traverse the residual network (positive remaining capacity only).
    for (size_t head = 0; head < queue.size(); ++head) {
        int u = queue[head];
        for (int edgeId : net.edgesFrom(u)) {
            const Edge& edge = net.edges()[edgeId];
            if (edge.cap > 0 && !reachable[edge.to]) {
                reachable[edge.to] = true;
                queue.push_back(edge.to);
            }
        }
    }
    if (reachable[graph.sink])
        throw std::logic_error("Sink is still reachable: run maxFlow before extraction");

    std::vector<int> seam(graph.rows, -1);
    for (int row = 0; row < graph.rows; ++row) {
        for (int col = 0; col < graph.cols; ++col) {
            if (reachable[graph.node(row, col)]) seam[row] = col;
        }
        if (seam[row] == -1)
            throw std::logic_error("Invalid cut: no reachable pixel in a row");
        if (row > 0 && std::abs(seam[row] - seam[row - 1]) > 1)
            throw std::logic_error("Invalid cut: seam jumps more than one column");
    }
    return seam;
}

std::vector<int> findVerticalGraphCutSeam(const Grid& energy) {
    SeamGraph graph = buildVerticalSeamGraph(energy);
    graph.net.maxFlow(graph.source, graph.sink);
    return extractVerticalSeam(graph);
}

} // namespace graphcut
