// ============================================================================
 // resize_test_image.cpp -- Create a small image for Graph Cut testing
 // Owner: Kervin (Graph Cut Part 2).
 //
 // Uses nearest-neighbour scaling to prepare a smaller test image.
 // Does not modify the original image or use seam carving.
 // ============================================================================

 #include "image_utils.hpp"

 #include <algorithm>
 #include <iostream>
 #include <stdexcept>

 int main() {
     try {
         const Image original =
             loadImage("assets/images/synthetic_scene.png");

         // Preserve aspect ratio, with maximum dimension of 80 pixels.
         const int maxDimension = std::max(original.rows, original.cols);
         const double scale = std::min(1.0, 80.0 / maxDimension);

         const int newRows = std::max(1, static_cast<int>(original.rows * scale));
         const int newCols = std::max(1, static_cast<int>(original.cols * scale));

         Image resized = makeImage(newRows, newCols, original.channels);

         // Nearest-neighbour scaling.
         for (int r = 0; r < newRows; ++r) {
             for (int c = 0; c < newCols; ++c) {
                 const int sourceRow =
                     std::min(original.rows - 1,
                              static_cast<int>(r / scale));

                 const int sourceCol =
                     std::min(original.cols - 1,
                              static_cast<int>(c / scale));

                 for (int k = 0; k < original.channels; ++k) {
                     resized.at(r, c, k) =
                         original.at(sourceRow, sourceCol, k);
                 }
             }
         }

         saveImage("assets/images/test_small.png", resized);

         std::cout << "Original: "
                   << original.cols << "x" << original.rows << '\n';

         std::cout << "Resized: "
                   << resized.cols << "x" << resized.rows << '\n';

         std::cout << "Saved assets/images/test_small.png\n";

         return 0;
     }
     catch (const std::exception& e) {
         std::cerr << "FAIL: " << e.what() << '\n';
         return 1;
     }
 }
