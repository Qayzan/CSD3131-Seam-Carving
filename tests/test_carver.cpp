// ============================================================================
// test_carver.cpp  --  the carving loop with mock seam finders (item 1, FF)
//
// The real algorithms belong to teammates, so these tests drive carve() with
// mock finders whose answers are known exactly:
//   * firstLine   -- always removes column 0 / row 0, so carving must equal cropping;
//   * exhaustive  -- tries every seam (test_helpers.hpp), the true optimum;
//   * broken ones -- deliberately break the contract, carve() must refuse them.
// ============================================================================
#include <cmath>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "carver.hpp"
#include "check.hpp"
#include "energy.hpp"
#include "image_utils.hpp"
#include "seam_ops.hpp"
#include "test_helpers.hpp"

using testing_helpers::bruteForceSeam;
using testing_helpers::randomImage;

namespace {

const Orientation V = Orientation::Vertical;
const Orientation H = Orientation::Horizontal;

Seam firstLine(const EnergyMap& energy, Orientation o) {
    Seam s;
    s.orientation = o;
    s.indices.assign(static_cast<std::size_t>(o == V ? energy.rows : energy.cols), 0);
    s.energy = seamEnergy(energy, s);
    return s;
}

Image crop(const Image& image, int top, int left) {
    Image out = makeImage(image.rows - top, image.cols - left, image.channels);
    for (int r = 0; r < out.rows; ++r)
        for (int c = 0; c < out.cols; ++c)
            for (int k = 0; k < image.channels; ++k) out.at(r, c, k) = image.at(r + top, c + left, k);
    return out;
}

Image testImage(int seed, int rows = 6, int cols = 7, int channels = 3) {
    std::mt19937 rng(static_cast<unsigned>(seed));
    return randomImage(rng, rows, cols, channels);
}

void testReachesTargetSize() {
    const Image image = testImage(1);
    for (CarveOrder order : {CarveOrder::WidthFirst, CarveOrder::HeightFirst, CarveOrder::Alternate}) {
        const int targets[][2] = {{6, 7}, {6, 4}, {3, 7}, {2, 3}, {1, 1}};
        for (const auto& t : targets) {
            const CarveResult result = carve(image, t[0], t[1], bruteForceSeam, order);
            CHECK(result.image.rows == t[0] && result.image.cols == t[1] && result.image.channels == 3);
            CHECK(result.image.pixels.size() == static_cast<std::size_t>(t[0]) * t[1] * 3);
            CHECK(result.seams.size() == static_cast<std::size_t>((6 - t[0]) + (7 - t[1])));
        }
    }
}

void testGreyImagesWork() {
    const CarveResult result = carve(testImage(2, 5, 6, 1), 3, 4, bruteForceSeam);
    CHECK(result.image.rows == 3 && result.image.cols == 4 && result.image.channels == 1);
}

void testNoChange() {
    const Image image = testImage(3);
    const CarveResult result = carve(image, 6, 7, bruteForceSeam, CarveOrder::WidthFirst, true);
    CHECK(result.seams.empty());
    CHECK(result.image.pixels == image.pixels);
    CHECK(result.frames.size() == 1);
}

void testInvalidArguments() {
    const Image image = testImage(4);
    CHECK_THROWS(std::invalid_argument, carve(image, 7, 7, bruteForceSeam));   // taller than the image
    CHECK_THROWS(std::invalid_argument, carve(image, 6, 8, bruteForceSeam));   // wider
    CHECK_THROWS(std::invalid_argument, carve(image, 0, 5, bruteForceSeam));
    CHECK_THROWS(std::invalid_argument, carve(image, 5, -1, bruteForceSeam));
    CHECK_THROWS(std::invalid_argument, carve(image, 5, 5, SeamFinder{}));     // no finder
    CHECK_THROWS(std::invalid_argument, carve(image, 5, 5, bruteForceSeam, CarveOrder::WidthFirst, false,
                                              EnergyFunction{}));
    Image bad = image;
    bad.pixels.pop_back();
    CHECK_THROWS(std::invalid_argument, carve(bad, 5, 5, bruteForceSeam));
}

void testRemovalPlans() {
    CHECK(removalPlan(2, 1, CarveOrder::WidthFirst) == std::vector<Orientation>({V, V, H}));
    CHECK(removalPlan(2, 1, CarveOrder::HeightFirst) == std::vector<Orientation>({H, V, V}));
    CHECK(removalPlan(3, 1, CarveOrder::Alternate) == std::vector<Orientation>({V, H, V, V}));
    CHECK(removalPlan(1, 3, CarveOrder::Alternate) == std::vector<Orientation>({V, H, H, H}));
    CHECK(removalPlan(0, 0, CarveOrder::Alternate).empty());
    CHECK_THROWS(std::invalid_argument, removalPlan(-1, 0, CarveOrder::WidthFirst));
    for (CarveOrder order : {CarveOrder::WidthFirst, CarveOrder::HeightFirst, CarveOrder::Alternate}) {
        const CarveResult result = carve(testImage(5), 4, 4, bruteForceSeam, order);
        std::vector<Orientation> got;
        for (const Seam& s : result.seams) got.push_back(s.orientation);
        CHECK(got == removalPlan(3, 2, order));
    }
}

void testEnergyIsRecomputedOnTheCurrentImage() {
    // Brief Task 1(c): recalculate the energy map after every removal.
    std::vector<std::pair<int, int>> sizes;
    const EnergyFunction spy = [&sizes](const Image& current) {
        sizes.emplace_back(current.rows, current.cols);
        return sobelEnergy(current);
    };
    carve(testImage(6), 4, 4, bruteForceSeam, CarveOrder::WidthFirst, false, spy);
    // 3 vertical seams (7 -> 4 columns), then 2 horizontal (6 -> 4 rows), each on the image of that moment.
    const std::vector<std::pair<int, int>> expected = {{6, 7}, {6, 6}, {6, 5}, {6, 4}, {5, 4}};
    CHECK(sizes == expected);
}

void testEnergyActuallyChangesAfterRemoval() {
    // The map handed to the finder must be the energy of the CURRENT image, not a cut-down
    // copy of the first map: compare with sobelEnergy of each frame.
    bool allMatch = true;
    int calls = 0;
    const SeamFinder recordingFinder = [&calls](const EnergyMap& energy, Orientation o) {
        ++calls;
        return bruteForceSeam(energy, o);
    };
    const CarveResult result = carve(testImage(7), 5, 5, recordingFinder, CarveOrder::Alternate, true);
    CHECK(calls == 3);
    for (std::size_t k = 0; k + 1 < result.frames.size(); ++k) {
        const EnergyMap fresh = sobelEnergy(result.frames[k]);
        const Seam best = bruteForceSeam(fresh, result.seams[k].orientation);
        allMatch = allMatch && best.indices == result.seams[k].indices;
    }
    CHECK(allMatch);
}

void testWrongSizeEnergyIsRejected() {
    const EnergyFunction tooSmall = [](const Image& image) {
        EnergyMap e = sobelEnergy(image);
        return transpose(e);  // rows and cols swapped
    };
    CHECK_THROWS(std::invalid_argument, carve(testImage(8), 6, 6, bruteForceSeam, CarveOrder::WidthFirst,
                                              false, tooSmall));
}

void testAnyCallableIsAFinder() {
    const Image image = testImage(9);
    // Plain function: carving with "remove column 0 / row 0" must equal a top-left crop.
    CHECK(carve(image, 4, 5, firstLine).image.pixels == crop(image, 2, 2).pixels);
    // Lambda and stateful function object work the same way.
    int calls = 0;
    const auto lambda = [&calls](const EnergyMap& e, Orientation o) { ++calls; return firstLine(e, o); };
    CHECK(carve(image, 5, 5, lambda).image.pixels == crop(image, 1, 2).pixels);
    CHECK(calls == 3);
    struct Counter {
        int* count;
        Seam operator()(const EnergyMap& e, Orientation o) const { ++*count; return firstLine(e, o); }
    };
    int counted = 0;
    carve(image, 6, 4, Counter{&counted});
    CHECK(counted == 3);
}

void testBrokenFindersAreCaught() {
    const Image image = testImage(10);
    const SeamFinder outOfRange = [](const EnergyMap& e, Orientation o) {
        Seam s = firstLine(e, o);
        s.indices.assign(s.indices.size(), 99);
        return s;
    };
    const SeamFinder wrongEnergy = [](const EnergyMap& e, Orientation o) {
        Seam s = firstLine(e, o);
        s.energy += 1.0;
        return s;
    };
    const SeamFinder wrongOrientation = [](const EnergyMap& e, Orientation) { return firstLine(e, H); };
    const SeamFinder disconnected = [](const EnergyMap& e, Orientation o) {
        Seam s = firstLine(e, o);
        s.indices.back() = 3;
        s.energy = seamEnergy(e, firstLine(e, o));
        return s;
    };
    const SeamFinder tooShort = [](const EnergyMap& e, Orientation o) {
        Seam s = firstLine(e, o);
        s.indices.pop_back();
        return s;
    };
    for (const SeamFinder* f : {&outOfRange, &wrongEnergy, &wrongOrientation, &disconnected, &tooShort})
        CHECK_THROWS(std::invalid_argument, carve(image, 6, 6, *f));
}

void testTimings() {
    const CarveResult r = carve(testImage(11), 4, 5, bruteForceSeam);
    CHECK(r.timings.energy >= 0 && r.timings.findSeam >= 0 && r.timings.remove >= 0);
    CHECK(r.timings.findSeam > 0);  // the exhaustive search takes measurable time
    CHECK(r.timings.total + 1e-9 >= r.timings.energy + r.timings.findSeam + r.timings.remove);
}

void testFrames() {
    const Image image = testImage(12);
    const CarveResult r = carve(image, 4, 5, bruteForceSeam, CarveOrder::WidthFirst, true);
    CHECK(r.frames.size() == r.seams.size() + 1 && r.frames.size() == 5);
    CHECK(r.frames.front().pixels == image.pixels);
    CHECK(r.frames.back().pixels == r.image.pixels);
    const int expected[][2] = {{6, 7}, {6, 6}, {6, 5}, {5, 5}, {4, 5}};
    bool shapes = true;
    for (int k = 0; k < 5; ++k) shapes = shapes && r.frames[k].rows == expected[k][0] && r.frames[k].cols == expected[k][1];
    CHECK(shapes);
    CHECK(carve(image, 4, 5, bruteForceSeam).frames.empty());
}

void testOriginalCoordinatesWithCropFinder() {
    const CarveResult r = carve(testImage(13), 4, 5, firstLine);  // removes columns 0,1 then rows 0,1
    const auto pos = seamsInOriginalCoordinates(6, 7, r.seams);
    CHECK(pos.size() == 4);
    bool ok = true;
    for (int k = 0; k < 2; ++k)
        for (int r0 = 0; r0 < 6; ++r0) ok = ok && pos[k][r0] == std::make_pair(r0, k);
    for (int k = 0; k < 2; ++k)
        for (int c = 0; c < 5; ++c) ok = ok && pos[2 + k][c] == std::make_pair(k, c + 2);
    CHECK(ok);
}

void testOriginalCoordinatesCoverTheRemovedPixels() {
    const Image image = testImage(14);
    for (CarveOrder order : {CarveOrder::WidthFirst, CarveOrder::HeightFirst, CarveOrder::Alternate}) {
        const CarveResult r = carve(image, 4, 4, bruteForceSeam, order);
        std::set<std::pair<int, int>> removed;
        std::size_t count = 0;
        for (const auto& seamPos : seamsInOriginalCoordinates(6, 7, r.seams))
            for (const auto& p : seamPos) {
                removed.insert(p);
                ++count;
            }
        CHECK(removed.size() == count);              // no pixel removed twice
        CHECK(count == static_cast<std::size_t>(6 * 7 - 4 * 4));  // exactly the pixels that are gone
    }
    // With vertical seams only, the surviving pixels read row by row must be the result.
    const CarveResult vOnly = carve(image, 6, 4, bruteForceSeam);
    std::set<std::pair<int, int>> gone;
    for (const auto& seamPos : seamsInOriginalCoordinates(6, 7, vOnly.seams))
        for (const auto& p : seamPos) gone.insert(p);
    std::vector<std::uint8_t> survivors;
    for (int r = 0; r < 6; ++r)
        for (int c = 0; c < 7; ++c)
            if (!gone.count({r, c}))
                for (int k = 0; k < 3; ++k) survivors.push_back(image.at(r, c, k));
    CHECK(survivors == vOnly.image.pixels);
}

void testContentAwareWithOptimalSeams() {
    // Flat background with a high-contrast 3x2 block: 4 optimal vertical seams must all go
    // through the background, so every block pixel survives.
    Image image = makeImage(8, 12, 3, 120);
    for (int r = 3; r < 6; ++r)
        for (int c = 5; c < 7; ++c) {
            image.at(r, c, 0) = 220;
            image.at(r, c, 1) = 20;
            image.at(r, c, 2) = 20;
        }
    const CarveResult r = carve(image, 8, 8, bruteForceSeam);
    int red = 0;
    for (int row = 0; row < r.image.rows; ++row)
        for (int c = 0; c < r.image.cols; ++c) red += r.image.at(row, c, 0) == 220 ? 1 : 0;
    CHECK(red == 6);
}

}  // namespace

int main() {
    check::run("reaches exactly the target size (3 orders, 5 targets)", testReachesTargetSize);
    check::run("greyscale images work", testGreyImagesWork);
    check::run("target = current size removes nothing", testNoChange);
    check::run("invalid targets, finder, energy function, image", testInvalidArguments);
    check::run("removal plans and seam order", testRemovalPlans);
    check::run("energy recomputed on the current image before every seam", testEnergyIsRecomputedOnTheCurrentImage);
    check::run("each seam is optimal for the fresh energy of that step", testEnergyActuallyChangesAfterRemoval);
    check::run("energy map of the wrong size is rejected", testWrongSizeEnergyIsRejected);
    check::run("function / lambda / functor finders (crop equality)", testAnyCallableIsAFinder);
    check::run("contract-breaking finders are caught", testBrokenFindersAreCaught);
    check::run("timings are reported", testTimings);
    check::run("frames record every step", testFrames);
    check::run("seams mapped back to original coordinates (crop finder)", testOriginalCoordinatesWithCropFinder);
    check::run("original coordinates cover exactly the removed pixels", testOriginalCoordinatesCoverTheRemovedPixels);
    check::run("optimal seams avoid a high-contrast object", testContentAwareWithOptimalSeams);
    return check::summary("test_carver");
}
