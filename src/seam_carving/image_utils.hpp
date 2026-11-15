// ============================================================================
// image_utils.hpp  --  image files, greyscale conversion, transposing
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation).
//
// Image files are read and written with stb_image / stb_image_write
// (third_party/stb, public domain). The brief allows libraries for image I/O;
// everything that is part of seam carving itself is our own code.
// ============================================================================
#ifndef IMAGE_UTILS_HPP
#define IMAGE_UTILS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "interfaces.hpp"

// Largest image we accept (pixels). Protects against corrupt headers that
// claim absurd sizes and against int overflow in rows * cols * channels.
constexpr long long kMaxImagePixels = 100LL * 1000 * 1000;  // 100 megapixels

// A blank image filled with `value`. Throws std::invalid_argument on bad sizes.
Image makeImage(int rows, int cols, int channels, std::uint8_t value = 0);

// Reads PNG, JPEG, BMP, TGA, GIF (first frame), PSD, PNM, HDR or PIC.
//   * grey (and grey + alpha) files -> channels = 1
//   * colour (and colour + alpha)   -> channels = 3, RGB order
// Alpha is dropped (seam carving ignores transparency); 16-bit files are
// reduced to 8 bits by stb_image. UTF-8 paths work on Windows too.
// Throws std::runtime_error (missing, unreadable, corrupt or unsupported file).
Image loadImage(const std::string& path);

// Throws std::invalid_argument unless `path` ends in .png, .bmp, .tga, .jpg or
// .jpeg (any case). saveImage calls it first; programs can call it early, before
// doing slow work whose result could not be saved.
void checkOutputFormat(const std::string& path);

// Writes by file extension: .png, .bmp, .tga, .jpg / .jpeg (quality 95).
// Creates missing parent folders.
// Throws std::invalid_argument for a bad image or an unknown extension,
// std::runtime_error if the file cannot be written.
void saveImage(const std::string& path, const Image& image);

// Luminance per pixel as double, row by row (rows * cols values):
//   grey: the value itself;  RGB: 0.299 R + 0.587 G + 0.114 B (ITU-R BT.601).
// Doubles, so later differences such as 200 - 250 = -50 never wrap around.
std::vector<double> toGrayscale(const Image& image);

// Swaps rows and columns: result.at(c, r, k) == image.at(r, c, k).
Image transpose(const Image& image);

// Copy of a grey image with 3 identical channels (RGB images are returned unchanged).
Image toRgb(const Image& image);

#endif  // IMAGE_UTILS_HPP
