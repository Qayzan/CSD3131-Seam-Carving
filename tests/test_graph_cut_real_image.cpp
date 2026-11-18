// ============================================================================
 // test_graph_cut_real_image.cpp -- Real-image DP vs Graph Cut comparison
 // Owner: Kervin (Graph Cut Part 2).
 //
 // Uses the shared carver to resize one image with both algorithms.
 // Saves the output images and reports their runtimes.
 // ============================================================================

 #include "carver.hpp"
 #include "dynamic_programming.hpp"
 #include "energy.hpp"
 #include "image_utils.hpp"
 #include "seam_extraction.hpp"

 #include <algorithm>
 #include <cmath>
 #include <iomanip>
 #include <iostream>
 #include <stdexcept>
 #include <string>
 #include <vector>

 // Adapter: connect Graph Cut to the team's shared SeamFinder interface.
 static Seam graphCutFinder(const EnergyMap& energy,
                            Orientation orientation) {
     checkEnergyMap(energy);

     // Horizontal seams become vertical seams after transposing.
     EnergyMap working = orientation == Orientation::Horizontal
                             ? transpose(energy)
                             : energy;

     graphcut::Grid grid(
         working.rows, std::vector<double>(working.cols)
     );

     for (int r = 0; r < working.rows; ++r)
         for (int c = 0; c < working.cols; ++c)
             grid[r][c] = working.at(r, c);

     Seam seam;
     seam.orientation = orientation;
     seam.indices = graphcut::findVerticalGraphCutSeam(grid);

     // The shared carver expects the original, unscaled energy.
     seam.energy = 0.0;

     for (int r = 0; r < working.rows; ++r)
         seam.energy += working.at(r, seam.indices[r]);

     return seam;
 }

 int main(int argc, char* argv[]) {
     try {
         if (argc != 2) {
             std::cerr << "Usage: test_graph_cut_real_image <image>\n";
             return 1;
         }

         Image input = loadImage(argv[1]);

         // Keep this first integration test small because Edmonds-Karp
         // is expensive on large images.
         if (input.rows > 120 || input.cols > 120)
             throw std::runtime_error(
                 "Please use a sample image no larger than 120x120"
             );

         int removeRows = std::min(3, input.rows - 1);
         int removeCols = std::min(5, input.cols - 1);

         int targetRows = input.rows - removeRows;
         int targetCols = input.cols - removeCols;

         // Same image, dimensions, energy function and removal order.
         CarveResult dpResult = carve(
             input, targetRows, targetCols,
             dp::findSeam, CarveOrder::WidthFirst
         );

         CarveResult graphResult = carve(
             input, targetRows, targetCols,
             graphCutFinder, CarveOrder::WidthFirst
         );

         if (dpResult.image.rows != graphResult.image.rows ||
             dpResult.image.cols != graphResult.image.cols)
             throw std::runtime_error("Output dimensions do not match");

         // Count whether the final images contain different pixels.
         std::size_t differences = 0;

         for (std::size_t i = 0; i < dpResult.image.pixels.size(); ++i)
             if (dpResult.image.pixels[i] != graphResult.image.pixels[i])
                 ++differences;

         saveImage("dp_result.png", dpResult.image);
         saveImage("graph_cut_result.png", graphResult.image);

         std::cout << std::fixed << std::setprecision(3);
         std::cout << "Input: " << input.cols << "x" << input.rows << '\n';
         std::cout << "Output: " << targetCols << "x" << targetRows << '\n';

         std::cout << "DP seam-finding time: "
                   << dpResult.timings.findSeam * 1000 << " ms\n";

         std::cout << "Graph Cut seam-finding time: "
                   << graphResult.timings.findSeam * 1000 << " ms\n";

         std::cout << "DP total time: "
                   << dpResult.timings.total * 1000 << " ms\n";

         std::cout << "Graph Cut total time: "
                   << graphResult.timings.total * 1000 << " ms\n";

         std::cout << "Different channel values: "
                   << differences << '\n';

         std::cout << "Saved dp_result.png and graph_cut_result.png\n";

         return 0;
     }
     catch (const std::exception& e) {
         std::cerr << "FAIL: " << e.what() << '\n';
         return 1;
     }
 }
