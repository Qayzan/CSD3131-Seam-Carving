// ============================================================================
 // test_seam_extraction.cpp -- Tests for Graph Cut seam extraction
 // Owner: Kervin (Graph Cut Part 2).
 //
 // Tests seam validity and compares Graph Cut's minimum seam cost
 // against a reference Dynamic Programming implementation.
 // Uses runtime checks so tests also work in Release builds.
 // ============================================================================

 #include "seam_extraction.hpp"

 #include <algorithm>
 #include <cstdlib>
 #include <iostream>
 #include <stdexcept>
 #include <string>
 #include <vector>

 using graphcut::Grid;

 // Reference DP computes the minimum possible seam cost using the
 // same scaled energy values as Graph Cut.
 static long long dpCost(const Grid& energy) {
     int rows = (int)energy.size();
     int cols = (int)energy[0].size();

     std::vector<long long> previous(cols), current(cols);

     for (int c = 0; c < cols; ++c)
         previous[c] = graphcut::toCapacity(energy[0][c]);

     for (int r = 1; r < rows; ++r) {
         for (int c = 0; c < cols; ++c) {
             long long best = previous[c];

             if (c > 0)
                 best = std::min(best, previous[c - 1]);

             if (c + 1 < cols)
                 best = std::min(best, previous[c + 1]);

             current[c] = best + graphcut::toCapacity(energy[r][c]);
         }

         previous.swap(current);
     }

     return *std::min_element(previous.begin(), previous.end());
 }

 // Throw an error if a test condition fails.
 // Unlike assert(), this still runs when NDEBUG is defined.
 static void require(bool condition, const std::string& message) {
     if (!condition)
         throw std::runtime_error(message);
 }

 static void check(const Grid& energy) {
     auto graph = graphcut::buildVerticalSeamGraph(energy);

     long long cutCost = graph.net.maxFlow(graph.source, graph.sink);
     auto seam = graphcut::extractVerticalSeam(graph);

     // Check that one pixel is selected from each row.
     require(seam.size() == energy.size(), "Incorrect seam length");

     long long seamCost = 0;

     for (int r = 0; r < (int)seam.size(); ++r) {
         // Check that each seam position is inside the image.
         require(seam[r] >= 0 && seam[r] < (int)energy[0].size(),
                 "Seam position out of bounds");

         // Adjacent seam pixels must differ by at most one column.
         if (r > 0) {
             require(std::abs(seam[r] - seam[r - 1]) <= 1,
                     "Invalid seam: adjacent pixels too far apart");
         }

         seamCost += graphcut::toCapacity(energy[r][seam[r]]);
     }

     // Compare Graph Cut's seam energy against max-flow and reference DP.
     require(seamCost == cutCost, "Seam cost differs from min-cut cost");
     require(seamCost == dpCost(energy), "Graph Cut and DP costs differ");

     std::cout << "PASS " << energy.size() << "x"
               << energy[0].size() << " seam=";

     for (int x : seam)
         std::cout << x << " ";

     std::cout << "cost=" << seamCost << '\n';
 }

 int main() {
     try {
         check({{1, 8, 8}, {1, 8, 8}, {1, 8, 8}});
         check({{8, 8, 1}, {8, 1, 8}, {1, 8, 8}});
         check({{4}});
         check({{7, 0, 4, 2}});
         check({{2}, {5}, {1}});
         check({{2, 1, 5}, {4, 1, 7}, {9, 1, 2}});

         std::cout << "All 6 tests passed.\n";
         return 0;
     }
     catch (const std::exception& e) {
         std::cerr << "FAIL: " << e.what() << '\n';
         return 1;
     }
 }
