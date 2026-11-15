// ============================================================================
// test_image_utils.cpp  --  image files, greyscale, transpose (item 1, FF)
// ============================================================================
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

#include "../third_party/stb/stb_image_write.h"  // declarations only; the library holds the code
#include "check.hpp"
#include "image_utils.hpp"
#include "test_helpers.hpp"

namespace fs = std::filesystem;
using testing_helpers::makeImageFrom;
using testing_helpers::randomImage;

namespace {

// Fresh folder for this program's files (deleted again at the end).
fs::path scratch() {
    static const fs::path dir = fs::temp_directory_path() / "seam_carving_test_image_utils";
    fs::create_directories(dir);
    return dir;
}
std::string file(const std::string& name) { return (scratch() / fs::u8path(name)).u8string(); }

bool samePixels(const Image& a, const Image& b) {
    return a.rows == b.rows && a.cols == b.cols && a.channels == b.channels && a.pixels == b.pixels;
}

void testMakeImage() {
    Image image = makeImage(2, 3, 3, 7);
    CHECK(image.rows == 2 && image.cols == 3 && image.channels == 3);
    CHECK(image.pixels.size() == 18);
    CHECK(image.at(1, 2, 2) == 7);
    image.at(1, 2, 0) = 200;
    CHECK(image.pixels[image.index(1, 2, 0)] == 200);
    CHECK(image.index(1, 2, 0) == 15);  // (1 * 3 + 2) * 3 + 0
    CHECK_THROWS(std::invalid_argument, makeImage(0, 3, 3));
    CHECK_THROWS(std::invalid_argument, makeImage(3, -1, 1));
    CHECK_THROWS(std::invalid_argument, makeImage(3, 3, 2));
    CHECK_THROWS(std::invalid_argument, makeImage(3, 3, 4));
    CHECK_THROWS(std::invalid_argument, makeImage(20000, 20000, 1));  // 400 MP > limit
}

void testCheckImageRejectsBadShapes() {
    Image image = makeImage(2, 2, 3);
    image.pixels.pop_back();  // size no longer rows * cols * channels
    CHECK_THROWS(std::invalid_argument, checkImage(image));
    CHECK_THROWS(std::invalid_argument, saveImage(file("bad.png"), image));
    CHECK_THROWS(std::invalid_argument, toGrayscale(image));
    Image twoChannels = makeImageFrom(1, 1, 2, {1, 2});
    CHECK_THROWS(std::invalid_argument, checkImage(twoChannels));
    Image empty;
    CHECK_THROWS(std::invalid_argument, checkImage(empty));
}

void testPngRoundTripRgbAndGrey() {
    std::mt19937 rng(1);
    const Image rgb = randomImage(rng, 5, 7, 3);
    saveImage(file("rgb.png"), rgb);
    CHECK(samePixels(loadImage(file("rgb.png")), rgb));

    const Image grey = randomImage(rng, 6, 3, 1);
    saveImage(file("grey.png"), grey);
    const Image loaded = loadImage(file("grey.png"));
    CHECK(loaded.channels == 1);
    CHECK(samePixels(loaded, grey));
}

void testRgbOrderIsKept() {
    Image red = makeImage(1, 2, 3, 0);
    red.at(0, 0, 0) = 255;  // pixel (0,0) pure red
    red.at(0, 1, 2) = 255;  // pixel (0,1) pure blue
    saveImage(file("order.png"), red);
    const Image back = loadImage(file("order.png"));
    CHECK(back.at(0, 0, 0) == 255 && back.at(0, 0, 1) == 0 && back.at(0, 0, 2) == 0);
    CHECK(back.at(0, 1, 0) == 0 && back.at(0, 1, 2) == 255);
}

void testOtherLosslessFormats() {
    std::mt19937 rng(2);
    const Image image = randomImage(rng, 4, 9, 3);
    for (const char* name : {"x.bmp", "x.tga", "X.PNG"}) {
        saveImage(file(name), image);
        CHECK(samePixels(loadImage(file(name)), image));
    }
}

void testJpegIsCloseButLossy() {
    Image image = makeImage(16, 16, 3, 120);  // flat colour compresses almost perfectly
    saveImage(file("flat.jpg"), image);
    const Image back = loadImage(file("flat.jpg"));
    CHECK(back.rows == 16 && back.cols == 16 && back.channels == 3);
    int worst = 0;
    for (std::size_t i = 0; i < back.pixels.size(); ++i)
        worst = std::max(worst, std::abs(int(back.pixels[i]) - int(image.pixels[i])));
    CHECK(worst <= 3);
}

void testAlphaIsDropped() {
    // A 1x2 RGBA file (blue, half transparent; red, opaque) written directly with stb.
    const std::uint8_t rgba[] = {0, 0, 255, 128, 255, 0, 0, 255};
    CHECK(stbi_write_png(file("alpha.png").c_str(), 2, 1, 4, rgba, 8) != 0);
    const Image image = loadImage(file("alpha.png"));
    CHECK(image.channels == 3 && image.rows == 1 && image.cols == 2);
    CHECK(image.at(0, 0, 2) == 255 && image.at(0, 1, 0) == 255);

    const std::uint8_t greyAlpha[] = {10, 0, 200, 255};  // grey + alpha -> grey
    CHECK(stbi_write_png(file("ga.png").c_str(), 2, 1, 2, greyAlpha, 4) != 0);
    const Image grey = loadImage(file("ga.png"));
    CHECK(grey.channels == 1 && grey.at(0, 0) == 10 && grey.at(0, 1) == 200);
}

void testNonAsciiPathAndNewFolder() {
    const Image image = makeImage(2, 3, 3, 77);
    const std::string path = file(u8"新しい folder é/图像 ü.png");
    saveImage(path, image);  // creates the folder too
    CHECK(samePixels(loadImage(path), image));
}

void testBadFiles() {
    CHECK_THROWS(std::runtime_error, loadImage(file("does_not_exist.png")));
    { std::ofstream(file("empty.png"), std::ios::binary); }
    CHECK_THROWS(std::runtime_error, loadImage(file("empty.png")));
    { std::ofstream(file("text.png"), std::ios::binary) << "this is not an image"; }
    CHECK_THROWS(std::runtime_error, loadImage(file("text.png")));

    // A real PNG cut in half: header fine, data missing.
    saveImage(file("full.png"), makeImage(32, 32, 3, 9));
    const auto size = fs::file_size(file("full.png"));
    fs::copy_file(file("full.png"), file("cut.png"), fs::copy_options::overwrite_existing);
    fs::resize_file(file("cut.png"), size / 2);
    CHECK_THROWS(std::runtime_error, loadImage(file("cut.png")));
}

void testUnsupportedSaveFormats() {
    const Image image = makeImage(2, 2, 1);
    CHECK_THROWS(std::invalid_argument, saveImage(file("no_extension"), image));
    CHECK_THROWS(std::invalid_argument, saveImage(file("picture.gif"), image));
    CHECK_THROWS(std::invalid_argument, saveImage(file("picture.webp"), image));
    CHECK_THROWS(std::invalid_argument, checkOutputFormat("result.gif"));
    checkOutputFormat("RESULT.JPEG");  // accepted, any case (no throw)
    checkOutputFormat("folder/out.png");
}

void testGrayscale() {
    const Image rgb = makeImageFrom(1, 4, 3, {255, 0, 0, 0, 255, 0, 0, 0, 255, 10, 20, 30});
    const std::vector<double> g = toGrayscale(rgb);
    CHECK(g.size() == 4);
    CHECK_NEAR(g[0], 0.299 * 255, 1e-12);
    CHECK_NEAR(g[1], 0.587 * 255, 1e-12);
    CHECK_NEAR(g[2], 0.114 * 255, 1e-12);
    CHECK_NEAR(g[3], 0.299 * 10 + 0.587 * 20 + 0.114 * 30, 1e-12);

    const Image grey = makeImageFrom(1, 3, 1, {0, 128, 255});
    const std::vector<double> h = toGrayscale(grey);
    CHECK(h[0] == 0.0 && h[1] == 128.0 && h[2] == 255.0);

    const std::vector<double> w = toGrayscale(makeImageFrom(1, 2, 1, {200, 250}));
    CHECK(w[0] - w[1] == -50.0);  // doubles: no 8-bit wrap-around (would be 206)
}

void testTransposeAndToRgb() {
    std::mt19937 rng(3);
    const Image image = randomImage(rng, 3, 5, 3);
    const Image t = transpose(image);
    CHECK(t.rows == 5 && t.cols == 3 && t.channels == 3);
    bool same = true;
    for (int r = 0; r < image.rows; ++r)
        for (int c = 0; c < image.cols; ++c)
            for (int k = 0; k < 3; ++k) same = same && t.at(c, r, k) == image.at(r, c, k);
    CHECK(same);
    CHECK(samePixels(transpose(t), image));  // transposing twice gives the original

    const Image grey = makeImageFrom(1, 2, 1, {5, 9});
    const Image rgb = toRgb(grey);
    CHECK(rgb.channels == 3 && rgb.at(0, 1, 0) == 9 && rgb.at(0, 1, 1) == 9 && rgb.at(0, 1, 2) == 9);
    CHECK(samePixels(toRgb(image), image));
}

}  // namespace

int main() {
    check::run("makeImage builds and indexes images", testMakeImage);
    check::run("checkImage rejects bad shapes", testCheckImageRejectsBadShapes);
    check::run("PNG round trip (RGB and grey)", testPngRoundTripRgbAndGrey);
    check::run("RGB channel order is kept", testRgbOrderIsKept);
    check::run("BMP / TGA round trips, upper-case extension", testOtherLosslessFormats);
    check::run("JPEG is close to the original", testJpegIsCloseButLossy);
    check::run("alpha channel is dropped", testAlphaIsDropped);
    check::run("non-ASCII path and new folder", testNonAsciiPathAndNewFolder);
    check::run("missing / empty / corrupt / truncated files throw", testBadFiles);
    check::run("unsupported output formats throw", testUnsupportedSaveFormats);
    check::run("greyscale conversion (BT.601, no wrap-around)", testGrayscale);
    check::run("transpose and toRgb", testTransposeAndToRgb);
    std::error_code ignored;
    fs::remove_all(scratch(), ignored);
    return check::summary("test_image_utils");
}
