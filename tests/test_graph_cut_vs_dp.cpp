
 // ============================================================================
 // test_graph_cut_vs_dp.cpp -- Compare Graph Cut against the team's actual DP
 // Owner: Kervin (Graph Cut Part 2).
 //
 // Tests vertical and horizontal seam finding using identical energy maps.
 // Checks that both algorithms produce valid seams with the same minimum cost.
 // Uses runtime checks that also work in Release builds.
 // ============================================================================

 #include "dynamic_programming.hpp"
 #include "seam_extraction.hpp"
 #include "interfaces.hpp"
 #include "energy.hpp"

 #include <cmath>
 #include <iostream>
 #include <stdexcept>
 #include <vector>

 // Convert the team's flat EnergyMap into Graph Cut's 2D Grid.
 static graphcut::Grid toGraphCutGrid(const EnergyMap& energy) {
     graphcut::Grid grid(
         energy.rows,
         std::vector<double>(energy.cols)
     );

     for (int r = 0; r < energy.rows; ++r) {
         for (int c = 0; c < energy.cols; ++c) {
             grid[r][c] = energy.at(r, c);
         }
     }

     return grid;
 }

 // Calculate the cost of a vertical seam using the original energy map.
 // For horizontal seams, pass the transposed map instead.
 static double calculateCost(const EnergyMap& energy,
                             const std::vector<int>& seam) {
     if (seam.size() != static_cast<std::size_t>(energy.rows))
         throw std::runtime_error("Incorrect seam length");

     double total = 0.0;

     for (int r = 0; r < energy.rows; ++r) {
         int c = seam[r];

         if (c < 0 || c >= energy.cols)
             throw std::runtime_error("Seam out of bounds");

         if (r > 0 && std::abs(c - seam[r - 1]) > 1)
             throw std::runtime_error("Invalid seam adjacency");

         total += energy.at(r, c);
     }

     return total;
 }

 // Compare vertical seams using the team's actual DP implementation.
 static void compareVertical(const EnergyMap& energy) {
     checkEnergyMap(energy);

     // Run Dynamic Programming.
     Seam dpSeam = dp::findSeam(
         energy, Orientation::Vertical
     );

     // Run Graph Cut.
     auto graphSeam = graphcut::findVerticalGraphCutSeam(
         toGraphCutGrid(energy)
     );

     double dpCost = calculateCost(energy, dpSeam.indices);
     double graphCost = calculateCost(energy, graphSeam);

     if (std::abs(dpSeam.energy - dpCost) > 1e-9)
         throw std::runtime_error("Incorrect vertical DP cost");

     // Integer-valued test energies avoid scaling differences.
     if (std::abs(dpCost - graphCost) > 1e-9)
         throw std::runtime_error(
             "Vertical Graph Cut and DP costs differ"
         );

     std::cout << "PASS vertical "
               << energy.rows << "x" << energy.cols
               << " | DP cost: " << dpCost
               << " | Graph Cut cost: " << graphCost
               << '\n';
 }

 // Compare horizontal seams using transpose.
 static void compareHorizontal(const EnergyMap& energy) {
     checkEnergyMap(energy);

     // DP finds a horizontal seam directly.
     Seam dpSeam = dp::findSeam(
         energy, Orientation::Horizontal
     );

     // A horizontal seam becomes a vertical seam after transposing.
     EnergyMap transposed = transpose(energy);

     auto graphSeam = graphcut::findVerticalGraphCutSeam(
         toGraphCutGrid(transposed)
     );

     // In the transposed map, each seam index represents a column.
     // In the original map, it represents a row.
     double dpCost = calculateCost(transposed, dpSeam.indices);
     double graphCost = calculateCost(transposed, graphSeam);

     if (std::abs(dpSeam.energy - dpCost) > 1e-9)
         throw std::runtime_error("Incorrect horizontal DP cost");

     if (std::abs(dpCost - graphCost) > 1e-9)
         throw std::runtime_error(
             "Horizontal Graph Cut and DP costs differ"
         );

     std::cout << "PASS horizontal "
               << energy.rows << "x" << energy.cols
               << " | DP cost: " << dpCost
               << " | Graph Cut cost: " << graphCost
               << '\n';
 }

 int main() {
     try {
         // Vertical seam tests.
         compareVertical(EnergyMap{3, 3, {
             1, 8, 8,
             1, 8, 8,
             1, 8, 8
         }});

         compareVertical(EnergyMap{3, 3, {
             8, 8, 1,
             8, 1, 8,
             1, 8, 8
         }});

         compareVertical(EnergyMap{3, 3, {
             2, 1, 5,
             4, 1, 7,
             9, 1, 2
         }});

         compareVertical(EnergyMap{1, 4, {
             7, 0, 4, 2
         }});

         compareVertical(EnergyMap{3, 1, {
             2, 5, 1
         }});

         // Horizontal seam tests.
         compareHorizontal(EnergyMap{3, 4, {
             1, 2, 8, 8,
             8, 1, 2, 8,
             8, 8, 1, 2
         }});

         compareHorizontal(EnergyMap{2, 3, {
             1, 8, 1,
             8, 1, 8
         }});

         compareHorizontal(EnergyMap{1, 4, {
             3, 2, 4, 1
         }});

         std::cout << "All 8 DP comparison tests passed.\n";
         return 0;
     }
     catch (const std::exception& e) {
         std::cerr << "FAIL: " << e.what() << '\n';
         return 1;
     }
 }
