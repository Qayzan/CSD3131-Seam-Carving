// ============================================================================
 // benchmark_graph_cut_vs_dp.cpp -- Graph Cut vs Dynamic Programming benchmark
 // Owner: Kervin (Graph Cut Part 2).
 //
 // Compares the team's actual DP and Graph Cut implementations using
 // identical synthetic energy maps. Each test is repeated five times,
 // and the median runtime is reported.
 // ============================================================================

 #include "dynamic_programming.hpp"
 #include "seam_extraction.hpp"
 #include "interfaces.hpp"

 #include <algorithm>
 #include <chrono>
 #include <cmath>
 #include <iomanip>
 #include <iostream>
 #include <random>
 #include <stdexcept>
 #include <vector>

 using Clock = std::chrono::steady_clock;

 // Convert the shared EnergyMap into Graph Cut's grid format.
 static graphcut::Grid toGrid(const EnergyMap& energy) {
     graphcut::Grid grid(
         energy.rows, std::vector<double>(energy.cols)
     );

     for (int r = 0; r < energy.rows; ++r)
         for (int c = 0; c < energy.cols; ++c)
             grid[r][c] = energy.at(r, c);

     return grid;
 }

 // Calculate the cost of a vertical seam using the original energy map.
 static double seamCost(const EnergyMap& energy,
                        const std::vector<int>& seam) {
     if (seam.size() != static_cast<std::size_t>(energy.rows))
         throw std::runtime_error("Invalid seam length");

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

 static double elapsedMs(Clock::time_point start,
                         Clock::time_point end) {
     return std::chrono::duration<double, std::milli>(
         end - start
     ).count();
 }

 // Return the median of five measurements.
 static double median(std::vector<double> times) {
     std::sort(times.begin(), times.end());
     return times[times.size() / 2];
 }

 static void benchmark(int size) {
     // Fixed random seed and integer energy values ensure
     // both algorithms receive identical, reproducible inputs.
     std::mt19937 rng(42);
     std::uniform_int_distribution<int> distribution(1, 255);

     EnergyMap energy;
     energy.rows = size;
     energy.cols = size;
     energy.values.resize(size * size);

     for (double& value : energy.values)
         value = distribution(rng);

     // Prepare the graph input before timing.
     graphcut::Grid grid = toGrid(energy);

     std::vector<double> dpTimes;
     std::vector<double> graphTimes;

     for (int run = 0; run < 5; ++run) {
         // Time the team's actual DP seam finder.
         auto startDP = Clock::now();

         Seam dpSeam = dp::findSeam(
             energy, Orientation::Vertical
         );

         auto endDP = Clock::now();

         // Time graph construction, max-flow and seam extraction.
         auto startGraph = Clock::now();

         auto graph = graphcut::buildVerticalSeamGraph(grid);
         graph.net.maxFlow(graph.source, graph.sink);
         auto graphSeam = graphcut::extractVerticalSeam(graph);

         auto endGraph = Clock::now();

         // Validate the results outside the timed sections.
         double dpCost = seamCost(energy, dpSeam.indices);
         double graphCost = seamCost(energy, graphSeam);

         if (std::abs(dpSeam.energy - dpCost) > 1e-9 ||
             std::abs(dpCost - graphCost) > 1e-9)
             throw std::runtime_error(
                 "DP and Graph Cut seam costs differ"
             );

         dpTimes.push_back(elapsedMs(startDP, endDP));
         graphTimes.push_back(elapsedMs(startGraph, endGraph));
     }

     double dpMedian = median(dpTimes);
     double graphMedian = median(graphTimes);

     std::cout << std::fixed << std::setprecision(3)
               << size << "x" << size
               << " | DP: " << dpMedian << " ms"
               << " | Graph Cut: " << graphMedian << " ms"
               << " | Ratio: " << graphMedian / dpMedian << "x"
               << " | PASS\n";
 }

 int main() {
     try {
         for (int size : {10, 20, 30, 40})
             benchmark(size);

         std::cout << "All DP vs Graph Cut benchmarks passed.\n";
         return 0;
     }
     catch (const std::exception& e) {
         std::cerr << "FAIL: " << e.what() << '\n';
         return 1;
     }
 }
