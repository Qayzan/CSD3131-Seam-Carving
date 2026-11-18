// ============================================================================
// seam_extraction.hpp -- Graph Cut seam extraction declarations
// Owner: Kervin (Graph Cut Part 2).
//
// Declares the functions used to recover the minimum-energy seam
// after the maximum-flow calculation.
// ============================================================================

#ifndef SEAM_EXTRACTION_HPP
#define SEAM_EXTRACTION_HPP

#include "seam_graph.hpp"
#include <vector>

namespace graphcut {
// Run this only after net.maxFlow(source, sink).
// Returns the zero-based column index of the seam pixel for each row.
std::vector<int> extractVerticalSeam(const SeamGraph& graph);

// Convenience wrapper: builds graph, finds max flow, then extracts seam.
std::vector<int> findVerticalGraphCutSeam(const Grid& energy);
}

#endif
