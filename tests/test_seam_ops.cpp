// ============================================================================
// test_seam_ops.cpp  --  seam validation, energy, removal, highlighting (item 1, FF)
// ============================================================================
#include <random>
#include <stdexcept>
#include <vector>

#include "check.hpp"
#include "image_utils.hpp"
#include "seam_ops.hpp"
#include "test_helpers.hpp"

using testing_helpers::makeImageFrom;
using testing_helpers::makeMap;
using testing_helpers::randomImage;
using testing_helpers::randomSeam;

namespace {

const Orientation V = Orientation::Vertical;
const Orientation H = Orientation::Horizontal;

Seam seam(Orientation o, std::vector<int> indices, double energy = 0.0) {
    Seam s;
    s.orientation = o;
    s.indices = std::move(indices);
    s.energy = energy;
    return s;
}

// The 4x5 worked example (docs/algorithm_design.md 2.5); its best vertical seam is [1,1,2,3] = 7.
EnergyMap worked() {
    return makeMap(4, 5, {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 1, 9, 7, 2, 6, 5, 3, 5});
}

// Slow, obviously-correct removal used as the reference.
Image slowRemove(const Image& image, const Seam& s) {
    Image out;
    out.channels = image.channels;
    if (s.orientation == V) {
        out.rows = image.rows;
        out.cols = image.cols - 1;
        for (int r = 0; r < image.rows; ++r)
            for (int c = 0; c < image.cols; ++c)
                if (c != s.indices[r])
                    for (int k = 0; k < image.channels; ++k) out.pixels.push_back(image.at(r, c, k));
    } else {
        out.rows = image.rows - 1;
        out.cols = image.cols;
        out.pixels.resize(static_cast<std::size_t>(out.rows) * out.cols * out.channels);
        for (int c = 0; c < image.cols; ++c) {
            int dst = 0;
            for (int r = 0; r < image.rows; ++r) {
                if (r == s.indices[c]) continue;
                for (int k = 0; k < image.channels; ++k) out.at(dst, c, k) = image.at(r, c, k);
                ++dst;
            }
        }
    }
    return out;
}

void testValidSeams() {
    CHECK(isValidSeam(seam(V, {1, 1, 2, 3}), 4, 5));
    CHECK(isValidSeam(seam(V, {0, 0, 0, 0}), 4, 5));  // left border
    CHECK(isValidSeam(seam(V, {4, 4, 4, 4}), 4, 5));  // right border
    CHECK(isValidSeam(seam(V, {0, 1, 2, 3}), 4, 5));  // diagonal
    CHECK(isValidSeam(seam(H, {0, 1, 2, 3, 3}), 4, 5));
    CHECK(isValidSeam(seam(V, {0}), 1, 1));
    CHECK(seamProblem(seam(V, {1, 1, 2, 3}), 4, 5).empty());
}

void testInvalidSeams() {
    CHECK(seamProblem(seam(V, {1, 1, 2}), 4, 5).find("needs 4 entries") != std::string::npos);
    CHECK(seamProblem(seam(H, {1, 1, 2, 3}), 4, 5).find("needs 5 entries") != std::string::npos);
    CHECK(seamProblem(seam(V, {1, 1, 2, 5}), 4, 5).find("leaves the picture") != std::string::npos);
    CHECK(seamProblem(seam(V, {-1, 0, 0, 0}), 4, 5).find("leaves the picture") != std::string::npos);
    CHECK(seamProblem(seam(H, {0, 1, 2, 3, 4}), 4, 5).find("leaves the picture") != std::string::npos);
    CHECK(seamProblem(seam(V, {1, 3, 3, 3}), 4, 5).find("not connected") != std::string::npos);
    CHECK(seamProblem(seam(H, {0, 0, 2, 2, 2}), 4, 5).find("not connected") != std::string::npos);
    CHECK(!isValidSeam(seam(V, {}), 4, 5));
    CHECK(!isValidSeam(seam(V, {0}), 0, 5));
    CHECK_THROWS(std::invalid_argument, validateSeam(seam(V, {1, 3, 3, 3}), 4, 5));
}

void testSeamEnergy() {
    CHECK(seamEnergy(worked(), seam(V, {1, 1, 2, 3})) == 7.0);      // 1 + 2 + 1 + 3
    CHECK(seamEnergy(worked(), seam(H, {0, 1, 2, 3, 3})) == 14.0);  // 3 + 2 + 1 + 3 + 5
    CHECK_THROWS(std::invalid_argument, seamEnergy(worked(), seam(V, {1, 1, 2})));
    validateSeamEnergy(seam(V, {1, 1, 2, 3}, 7.0), worked());  // no throw
    CHECK_THROWS(std::invalid_argument, validateSeamEnergy(seam(V, {1, 1, 2, 3}, 8.0), worked()));
    validateSeamEnergy(seam(V, {1, 1, 2, 3}, 7.0 + 1e-12), worked());  // rounding is tolerated
}

void testSeamPixels() {
    const auto v = seamPixels(seam(V, {1, 1, 2}));
    CHECK(v == (std::vector<std::pair<int, int>>{{0, 1}, {1, 1}, {2, 2}}));
    const auto h = seamPixels(seam(H, {2, 1, 1}));
    CHECK(h == (std::vector<std::pair<int, int>>{{2, 0}, {1, 1}, {1, 2}}));
}

void testRemoveByHand() {
    // 0  1  2  3      vertical seam [1, 2, 3]  ->   0  2  3
    // 4  5  6  7                                    4  5  7
    // 8  9 10 11                                    8  9 10
    const Image image = makeImageFrom(3, 4, 1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    const Image v = removeSeam(image, seam(V, {1, 2, 3}));
    CHECK(v.rows == 3 && v.cols == 3 && v.channels == 1);
    CHECK(v.pixels == std::vector<std::uint8_t>({0, 2, 3, 4, 5, 7, 8, 9, 10}));
    // horizontal seam [0, 1, 1, 2]: remove row 0 from col 0, row 1 from cols 1-2, row 2 from col 3
    const Image h = removeSeam(image, seam(H, {0, 1, 1, 2}));
    CHECK(h.rows == 2 && h.cols == 4);
    CHECK(h.pixels == std::vector<std::uint8_t>({4, 1, 2, 3, 8, 9, 10, 7}));
}

void testRgbPixelsMoveTogether() {
    // 2x3 RGB; channel values (v, 10v, 20v) so a mixed-up channel would show.
    Image image = makeImage(2, 3, 3);
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 3; ++c) {
            const int v = r * 3 + c + 1;
            image.at(r, c, 0) = static_cast<std::uint8_t>(v);
            image.at(r, c, 1) = static_cast<std::uint8_t>(10 * v);
            image.at(r, c, 2) = static_cast<std::uint8_t>(20 * v);
        }
    const Image out = removeSeam(image, seam(V, {0, 1}));
    CHECK(out.at(0, 0, 0) == 2 && out.at(0, 0, 1) == 20 && out.at(0, 0, 2) == 40);
    CHECK(out.at(1, 0, 0) == 4 && out.at(1, 1, 0) == 6);
}

void testRemovalMatchesSlowReference() {
    std::mt19937 rng(21);
    const int shapes[][3] = {{5, 8, 1}, {8, 5, 3}, {1, 6, 3}, {6, 2, 1}, {2, 2, 3}, {6, 1, 1}, {9, 13, 3}};
    int cases = 0;
    for (const auto& s : shapes)
        for (Orientation o : {V, H}) {
            if ((o == V && s[1] == 1) || (o == H && s[0] == 1)) continue;  // nothing would be left
            for (int k = 0; k < 20; ++k) {
                const Image image = randomImage(rng, s[0], s[1], s[2]);
                const Image copy = image;
                const Seam sm = randomSeam(rng, s[0], s[1], o);
                const Image out = removeSeam(image, sm);
                const Image ref = slowRemove(image, sm);
                CHECK(out.rows == ref.rows && out.cols == ref.cols && out.channels == ref.channels);
                CHECK(out.pixels == ref.pixels);  // every other pixel kept, in order
                CHECK(image.pixels == copy.pixels);  // input untouched
                ++cases;
            }
        }
    CHECK(cases == 240);  // 12 shape/orientation combinations x 20 seams
}

void testEnergyMapRemoval() {
    // Column by column: remove row 0 from column 0, row 1 from column 1, row 2 from column 2,
    // row 3 from columns 3 and 4; everything below the removed pixel moves up.
    const EnergyMap out = removeSeam(worked(), seam(H, {0, 1, 2, 3, 3}));
    CHECK(out.rows == 3 && out.cols == 5);
    CHECK(out.values == std::vector<double>({9, 1, 4, 1, 5, 5, 8, 6, 5, 3, 2, 6, 5, 9, 7}));
}

void testCannotRemoveLastColumnOrRow() {
    CHECK_THROWS(std::invalid_argument, removeSeam(makeImage(3, 1, 1), seam(V, {0, 0, 0})));
    CHECK_THROWS(std::invalid_argument, removeSeam(makeImage(1, 3, 1), seam(H, {0, 0, 0})));
    CHECK_THROWS(std::invalid_argument, removeSeam(makeImage(4, 5, 1), seam(V, {0, 0, 0})));
    CHECK_THROWS(std::invalid_argument, removeSeam(makeImage(4, 5, 1), seam(V, {0, 2, 2, 2})));
}

void testRepeatedRemoval() {
    std::mt19937 rng(22);
    Image image = randomImage(rng, 7, 9, 3);
    for (int k = 1; k <= 3; ++k) {
        image = removeSeam(image, randomSeam(rng, image.rows, image.cols, V));
        CHECK(image.rows == 7 && image.cols == 9 - k);
    }
    for (int k = 1; k <= 3; ++k) {
        image = removeSeam(image, randomSeam(rng, image.rows, image.cols, H));
        CHECK(image.rows == 7 - k && image.cols == 6);
    }
}

void testHighlight() {
    std::mt19937 rng(23);
    const Image image = randomImage(rng, 4, 5, 3);
    const Seam s = seam(V, {1, 1, 2, 3});
    const Image out = highlightSeam(image, s, 255, 255, 0);
    bool ok = out.rows == 4 && out.cols == 5 && out.channels == 3;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 5; ++c)
            for (int k = 0; k < 3; ++k) {
                const bool onSeam = c == s.indices[r];
                const std::uint8_t expected = onSeam ? (k < 2 ? 255 : 0) : image.at(r, c, k);
                ok = ok && out.at(r, c, k) == expected;
            }
    CHECK(ok);
    const Image grey = highlightSeam(makeImage(3, 2, 1, 50), seam(H, {2, 1}));
    CHECK(grey.channels == 3 && grey.at(2, 0, 0) == 255 && grey.at(2, 0, 1) == 0 && grey.at(0, 0, 0) == 50);
    CHECK_THROWS(std::invalid_argument, highlightSeam(image, seam(V, {0, 0})));
}

}  // namespace

int main() {
    check::run("valid seams pass", testValidSeams);
    check::run("invalid seams are rejected with a reason", testInvalidSeams);
    check::run("seam energy and its check", testSeamEnergy);
    check::run("seam pixel coordinates", testSeamPixels);
    check::run("removal by hand (vertical and horizontal)", testRemoveByHand);
    check::run("RGB pixels move together", testRgbPixelsMoveTogether);
    check::run("removal equals a slow reference on 240 random seams", testRemovalMatchesSlowReference);
    check::run("energy-map removal", testEnergyMapRemoval);
    check::run("cannot remove the last column / row or a misfit seam", testCannotRemoveLastColumnOrRow);
    check::run("repeated removal shrinks one step at a time", testRepeatedRemoval);
    check::run("seam highlighting", testHighlight);
    return check::summary("test_seam_ops");
}
