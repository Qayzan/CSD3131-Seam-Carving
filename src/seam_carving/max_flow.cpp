// ============================================================================
// max_flow.cpp  --  Edmonds-Karp maximum flow (see max_flow.hpp)
// Owner: syazwani (Graph Cut part 1).
// ============================================================================
#include "max_flow.hpp"

#include <algorithm>
#include <limits>

namespace graphcut {

FlowNetwork::FlowNetwork(int nodeCount) : adj(nodeCount) {}

void FlowNetwork::addEdge(int from, int to, ll cap) {
    adj[from].push_back((int)edgeList.size());
    edgeList.push_back(Edge{to, cap});
    adj[to].push_back((int)edgeList.size());
    edgeList.push_back(Edge{from, 0});  // reverse edge starts empty
}

ll FlowNetwork::maxFlow(int s, int t) {
    ll total = 0;
    // parentEdge[v] = id of the edge used to reach v in this BFS (-1 = not reached yet).
    std::vector<int> parentEdge(adj.size());
    std::vector<int> queue;

    while (true) {
        // 1. BFS from s over edges with remaining capacity. BFS visits nodes in
        //    order of distance, so the path it finds to t is a shortest one.
        std::fill(parentEdge.begin(), parentEdge.end(), -1);
        queue.clear();
        queue.push_back(s);
        bool reachedT = false;
        for (size_t head = 0; head < queue.size() && !reachedT; ++head) {
            int u = queue[head];
            for (size_t k = 0; k < adj[u].size(); ++k) {
                int id = adj[u][k];
                const Edge& e = edgeList[id];
                if (e.cap > 0 && e.to != s && parentEdge[e.to] < 0) {
                    parentEdge[e.to] = id;
                    if (e.to == t) { reachedT = true; break; }
                    queue.push_back(e.to);
                }
            }
        }
        if (!reachedT) break;  // no augmenting path left: the flow is maximum

        // 2. Bottleneck = smallest remaining capacity on the path. Walk back
        //    from t to s: the node before v is the "to" of the reverse edge.
        ll bottleneck = std::numeric_limits<ll>::max();
        for (int v = t; v != s; v = edgeList[parentEdge[v] ^ 1].to)
            bottleneck = std::min(bottleneck, edgeList[parentEdge[v]].cap);

        // 3. Push the bottleneck along the path: forward edges lose capacity,
        //    their reverse edges gain it (so later paths can undo this flow).
        for (int v = t; v != s; v = edgeList[parentEdge[v] ^ 1].to) {
            edgeList[parentEdge[v]].cap -= bottleneck;
            edgeList[parentEdge[v] ^ 1].cap += bottleneck;
        }
        total += bottleneck;
    }
    return total;
}

}  // namespace graphcut
