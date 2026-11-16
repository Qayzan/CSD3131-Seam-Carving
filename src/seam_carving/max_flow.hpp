// ============================================================================
// max_flow.hpp  --  flow network + Edmonds-Karp maximum flow
// Owner: syazwani (Graph Cut part 1: graph construction + max-flow).
//
// Edmonds-Karp = Ford-Fulkerson where every augmenting path is a SHORTEST
// path (fewest edges), found by BFS in the residual graph. Choosing shortest
// paths bounds the number of augmentations by O(V*E), so the whole algorithm
// is O(V * E^2), whatever the capacities are.
//
// Every edge is stored together with its reverse ("residual") edge. Edge
// number k and edge number (k ^ 1) are partners, so the reverse of edge k is
// always k ^ 1. Real edges have even ids, their reverse partners odd ids.
// Edge::cap is the REMAINING (residual) capacity of that edge.
// ============================================================================
#ifndef MAX_FLOW_HPP
#define MAX_FLOW_HPP

#include <vector>

namespace graphcut {

typedef long long ll;

struct Edge {
    int to;
    ll cap;  // remaining (residual) capacity
};

class FlowNetwork {
public:
    explicit FlowNetwork(int nodeCount);

    // Adds edge from -> to with capacity cap, plus its reverse edge (capacity 0).
    void addEdge(int from, int to, ll cap);

    // Edmonds-Karp. Returns the value of the maximum s-t flow.
    // Afterwards edges() holds the residual capacities: no path from s to t
    // with remaining capacity is left. The seam extraction (KO) reads them.
    ll maxFlow(int s, int t);

    int nodeCount() const { return (int)adj.size(); }
    const std::vector<int>& edgesFrom(int u) const { return adj[u]; }  // ids of edges leaving u
    const std::vector<Edge>& edges() const { return edgeList; }

private:
    std::vector<Edge> edgeList;
    std::vector<std::vector<int> > adj;  // adj[u] = ids of edges leaving u
};

}  // namespace graphcut

#endif  // MAX_FLOW_HPP
