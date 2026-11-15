// ============================================================================
// test_energy.cpp  --  Sobel energy map (item 1, FF) and its D&C / separable
// variants (item 7, FF)
// ============================================================================
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <vector>

#include "check.hpp"
#include "energy.hpp"
#include "image_utils.hpp"
#include "test_helpers.hpp"

using testing_helpers::makeImageFrom;
using testing_helpers::randomImage;

namespace {

// ---- Independent reference -------------------------------------------------
// Written differently on purpose: explicit 3x3 kernel tables, an explicitly
// padded copy of the picture (mirror without repeating the edge), and a plain
// double loop over the kernel. If sobelEnergy used a wrong kernel weight,
// sign convention that matters, border rule or channel weighting, the two
// would disagree.
EnergyMap referenceSobel(const Image& image) {
    const int R = image.rows, C = image.cols;
    std::vector<double> gray(static_cast<std::size_t>(R) * C);
    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) {
            if (image.channels == 1) {
                gray[static_cast<std::size_t>(r) * C + c] = image.at(r, c);
            } else {
                gray[static_cast<std::size_t>(r) * C + c] =
                    0.299 * image.at(r, c, 0) + 0.587 * image.at(r, c, 1) + 0.114 * image.at(r, c, 2);
            }
        }
    // Padded copy, (R + 2) x (C + 2): mirror -1 -> 1 and n -> n - 2 (a 1-wide side mirrors onto itself).
    const auto mirror = [](int i, int n) { return n == 1 ? 0 : (i < 0 ? 1 : (i >= n ? n - 2 : i)); };
    std::vector<std::vector<double>> pad(static_cast<std::size_t>(R + 2), std::vector<double>(static_cast<std::size_t>(C + 2)));
    for (int r = -1; r <= R; ++r)
        for (int c = -1; c <= C; ++c)
            pad[r + 1][c + 1] = gray[static_cast<std::size_t>(mirror(r, R)) * C + mirror(c, C)];
    const int kx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int ky[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    EnergyMap out;
    out.rows = R;
    out.cols = C;
    out.values.resize(static_cast<std::size_t>(R) * C);
    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C; ++c) {
            double gx = 0.0, gy = 0.0;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) {
                    gx += kx[i][j] * pad[r + i][c + j];
                    gy += ky[i][j] * pad[r + i][c + j];
                }
            out.at(r, c) = std::fabs(gx) + std::fabs(gy);
        }
    return out;
}

double maxDifference(const EnergyMap& a, const EnergyMap& b) {
    if (a.rows != b.rows || a.cols != b.cols || a.values.size() != b.values.size()) return 1e300;
    double worst = 0.0;
    for (std::size_t i = 0; i < a.values.size(); ++i) worst = std::fmax(worst, std::fabs(a.values[i] - b.values[i]));
    return worst;
}

const int kShapes[][2] = {{1, 1}, {1, 6}, {6, 1}, {2, 2}, {2, 3}, {4, 5}, {5, 4}, {17, 23}, {64, 31}};

// ---- Tests ------------------------------------------------------------------

void testMatchesIndependentReference() {
    std::mt19937 rng(7);
    for (const auto& shape : kShapes)
        for (int channels : {1, 3}) {
            const Image image = randomImage(rng, shape[0], shape[1], channels);
            CHECK(maxDifference(sobelEnergy(image), referenceSobel(image)) <= 1e-9);
        }
}

void testSizeTypeAndSign() {
    std::mt19937 rng(8);
    for (const auto& shape : kShapes) {
        const EnergyMap e = sobelEnergy(randomImage(rng, shape[0], shape[1], 3));
        CHECK(e.rows == shape[0] && e.cols == shape[1]);
        CHECK(e.values.size() == static_cast<std::size_t>(shape[0]) * shape[1]);  // one value per pixel
        bool nonNegative = true;
        for (double v : e.values) nonNegative = nonNegative && v >= 0.0 && std::isfinite(v);
        CHECK(nonNegative);
    }
}

void testVerticalEdgeByHand() {
    // Grey picture, 3 rows of  0 0 10 10.
    // Column 1: Gx = (10-0)*1 + (10-0)*2 + (10-0)*1 = 40 (the right neighbour is 10, the left 0).
    // Column 2: the same, 40. Columns 0 and 3: the mirrored neighbour equals the real one,
    // so Gx = 0. Nothing changes from top to bottom, so Gy = 0 everywhere.
    const Image image = makeImageFrom(3, 4, 1, {0, 0, 10, 10, 0, 0, 10, 10, 0, 0, 10, 10});
    const EnergyMap e = sobelEnergy(image);
    const std::vector<double> expected = {0, 40, 40, 0, 0, 40, 40, 0, 0, 40, 40, 0};
    CHECK(e.values == expected);
}

void testSinglePointByHand() {
    // 3x3 grey, a single 9 in the centre:     0 0 0 / 0 9 0 / 0 0 0
    // Centre: left = right and up = down    -> Gx = Gy = 0          -> 0
    // (0,1) above the centre: Gx = 0 (symmetric); Gy uses rows -1 (mirrored = row 1) and 1:
    //        (0-0) + 2*(9-9) + (0-0) = 0    -> 0   (the mirror copies the bright row above too)
    // (1,0) left of the centre: Gx = (0-0) + 2*(9-9) + (0-0) = 0  (mirrored column -1 = column 1)
    // (0,0) corner: Gx row terms (0-0) + 2*(0-0) + (9-9) = 0, Gy similarly 0
    // So with reflect-101 a lone bright pixel in a 3x3 picture gives energy 0 everywhere:
    // every neighbour sees the bright pixel on BOTH sides. (Zero padding would not.)
    const Image image = makeImageFrom(3, 3, 1, {0, 0, 0, 0, 9, 0, 0, 0, 0});
    const EnergyMap e = sobelEnergy(image);
    bool allZero = true;
    for (double v : e.values) allZero = allZero && v == 0.0;
    CHECK(allZero);

    // In a 1x3 strip with the bright pixel at the end, the middle pixel sees it on one side:
    // row mirrors onto itself (Gy = 0); Gx = (9-0)*(1+2+1) = 36.
    const EnergyMap strip = sobelEnergy(makeImageFrom(1, 3, 1, {0, 0, 9}));
    CHECK(strip.values == std::vector<double>({0, 36, 0}));
}

void testFlatImageIsZeroIncludingBorder() {
    const EnergyMap e = sobelEnergy(makeImage(5, 6, 3, 200));
    bool allZero = true;
    for (double v : e.values) allZero = allZero && v == 0.0;
    CHECK(allZero);
}

void testNoEightBitWrapAround() {
    const Image up = makeImageFrom(3, 4, 1, {200, 200, 250, 250, 200, 200, 250, 250, 200, 200, 250, 250});
    const Image down = makeImageFrom(3, 4, 1, {250, 250, 200, 200, 250, 250, 200, 200, 250, 250, 200, 200});
    CHECK(sobelEnergy(up).values == sobelEnergy(down).values);
    CHECK(sobelEnergy(up).at(1, 1) == 4 * 50.0);
}

void testColourUsesLuminance() {
    Image image = makeImage(3, 4, 3, 0);
    for (int r = 0; r < 3; ++r)
        for (int c = 2; c < 4; ++c) image.at(r, c, 1) = 100;  // green step: luminance 58.7
    const EnergyMap e = sobelEnergy(image);
    CHECK_NEAR(e.at(1, 1), 4 * 58.7, 1e-9);
    CHECK_NEAR(e.at(1, 2), 4 * 58.7, 1e-9);
    CHECK(e.at(1, 0) == 0.0 && e.at(1, 3) == 0.0);
}

void testTransposeSymmetry() {
    // Gx of the picture = Gy of the transposed picture, so the energy of the transpose is the
    // transpose of the energy. The two kernels add their terms in a different order, so the
    // results can differ in the last bit on some compilers: compare with a tiny tolerance.
    std::mt19937 rng(9);
    for (const auto& shape : kShapes) {
        const Image image = randomImage(rng, shape[0], shape[1], 3);
        const EnergyMap a = sobelEnergy(transpose(image));
        const EnergyMap b = transpose(sobelEnergy(image));
        CHECK(a.rows == b.rows && a.cols == b.cols && a.values.size() == b.values.size());
        double worst = 0.0;
        for (std::size_t i = 0; i < a.values.size() && i < b.values.size(); ++i)
            worst = std::max(worst, std::fabs(a.values[i] - b.values[i]));
        CHECK(worst <= 1e-9);
    }
}

void testReflect101() {
    CHECK(reflect101(-1, 5) == 1);
    CHECK(reflect101(5, 5) == 3);
    CHECK(reflect101(0, 5) == 0 && reflect101(4, 5) == 4);
    CHECK(reflect101(-1, 2) == 1 && reflect101(2, 2) == 0);
    CHECK(reflect101(-1, 1) == 0 && reflect101(1, 1) == 0);
}

void testSeparableMatches() {
    std::mt19937 rng(10);
    for (const auto& shape : kShapes)
        for (int channels : {1, 3}) {
            const Image image = randomImage(rng, shape[0], shape[1], channels);
            CHECK(maxDifference(sobelEnergySeparable(image), sobelEnergy(image)) <= 1e-9);
        }
}

void testDivideAndConquerIsIdentical() {
    std::mt19937 rng(11);
    for (const auto& shape : kShapes) {
        const Image image = randomImage(rng, shape[0], shape[1], 3);
        const EnergyMap direct = sobelEnergy(image);
        for (int leaf : {1, 2, 7, 64, 1 << 20})
            CHECK(sobelEnergyDivideAndConquer(image, leaf).values == direct.values);  // bit-for-bit
    }
    CHECK_THROWS(std::invalid_argument, sobelEnergyDivideAndConquer(makeImage(2, 2, 1), 0));
}

void testEnergyMapTranspose() {
    EnergyMap e = testing_helpers::makeMap(2, 3, {1, 2, 3, 4, 5, 6});
    const EnergyMap t = transpose(e);
    CHECK(t.rows == 3 && t.cols == 2);
    CHECK(t.values == std::vector<double>({1, 4, 2, 5, 3, 6}));
    e.values.pop_back();
    CHECK_THROWS(std::invalid_argument, transpose(e));
}

void testInvalidImagesAreRejected() {
    Image bad = makeImage(2, 2, 3);
    bad.channels = 4;
    CHECK_THROWS(std::invalid_argument, sobelEnergy(bad));
    CHECK_THROWS(std::invalid_argument, sobelEnergy(Image{}));
}

}  // namespace

int main() {
    check::run("matches an independent reference (9 shapes, grey + RGB)", testMatchesIndependentReference);
    check::run("one non-negative value per pixel", testSizeTypeAndSign);
    check::run("hand-computed vertical edge", testVerticalEdgeByHand);
    check::run("hand-computed single bright pixel (border mirroring)", testSinglePointByHand);
    check::run("flat image is 0 everywhere, borders included", testFlatImageIsZeroIncludingBorder);
    check::run("no 8-bit wrap-around", testNoEightBitWrapAround);
    check::run("colour images use luminance", testColourUsesLuminance);
    check::run("energy(transpose) == transpose(energy)", testTransposeSymmetry);
    check::run("reflect101 border rule", testReflect101);
    check::run("separable Sobel gives the same map", testSeparableMatches);
    check::run("divide & conquer gives the identical map", testDivideAndConquerIsIdentical);
    check::run("EnergyMap transpose", testEnergyMapTranspose);
    check::run("invalid images are rejected", testInvalidImagesAreRejected);
    return check::summary("test_energy");
}
