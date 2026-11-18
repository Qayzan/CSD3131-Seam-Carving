// ============================================================================
// bindings.cpp  --  the seam carving library, compiled to WebAssembly for the
// interactive browser demo (web/index.html).
// Owner: FF (Fauzan Fauzy). Bonus 7(a) step-by-step seams, 7(b) user resizing.
//
// Nothing here re-implements an algorithm. The browser calls the SAME C++ code
// as the command-line demo and the tests: sobelEnergy, dp::findSeam,
// validateSeam, removeSeam and carve(). Greedy and graph cut are added by
// their owners (see "Teammates" below).
//
// A Carver object holds one picture. The page drives it one seam at a time:
//     findSeam(orientation, algorithm)   energy of the current picture + seam
//     pendingRGBA()                       the picture with that seam drawn in red
//     removePending()                     removes the seam (and remembers where it was)
// so it can show every seam before it disappears. compare() runs the whole
// resize with carve() for one algorithm, for the side-by-side comparison.
//
// Build: emcmake cmake -S web -B build-web && cmake --build build-web
// ============================================================================
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "carver.hpp"
#include "energy.hpp"
#include "image_utils.hpp"
#include "interfaces.hpp"
#include "seam_ops.hpp"

#include "dynamic_programming.hpp"  // FF: Question 1, wired in
#include "seam_extraction.hpp"  // KO: Graph Cut Part 2

// ---- Teammates: add your own seam finder here --------------------------------
// The demo is the shared base plus the DP. Each method's owner plugs in their
// own finder, so the work stays theirs:
//   1. include your header below (e.g. "greedy.hpp" or "graph_cut.hpp");
//   2. add one line to algorithmList() further down;
//   3. rebuild (see web/README.md). The page lists, animates and compares
//      every method in that list automatically.
// ------------------------------------------------------------------------------

using emscripten::val;

namespace {

using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

struct Algorithm {
    std::string id, label, note;
    SeamFinder finder;
};


 // ============================================================================
 // Graph Cut adapter -- connects Graph Cut to the shared SeamFinder interface.
 // Owner: Kervin (Graph Cut Part 2).
 //
 // Converts EnergyMap into Graph Cut's Grid format, handles horizontal seams
 // using transpose, and returns the seam in the shared team's format.
 // ============================================================================

 Seam graphCutFinder(const EnergyMap& energy, Orientation orientation) {
     checkEnergyMap(energy);

     // Horizontal seams become vertical seams after transposing.
     EnergyMap working = (orientation == Orientation::Horizontal)
                             ? transpose(energy)
                             : energy;

     // Convert the shared flat energy map into Graph Cut's 2D grid.
     graphcut::Grid grid(
         working.rows,
         std::vector<double>(working.cols)
     );

     for (int r = 0; r < working.rows; ++r) {
         for (int c = 0; c < working.cols; ++c) {
             grid[r][c] = working.at(r, c);
         }
     }

     // Run the existing Graph Cut implementation.
     Seam seam;
     seam.orientation = orientation;
     seam.indices = graphcut::findVerticalGraphCutSeam(grid);

     // The shared carver expects the original, unscaled seam energy.
     seam.energy = 0.0;

     for (int r = 0; r < working.rows; ++r) {
         seam.energy += working.at(r, seam.indices[r]);
     }

     return seam;
 }


const std::vector<Algorithm>& algorithmList() {
    static const std::vector<Algorithm> list = [] {
        std::vector<Algorithm> a;
        a.push_back({"dp", "Dynamic programming", "Optimal seam, \u0398(mn) per seam", dp::findSeam});
        // HM:  a.push_back({"greedy", "Greedy", "Cheapest neighbour row by row", greedy::findSeam});
        // KO:  a.push_back({"graphcut", "Graph cut", "Min-cut / max-flow, slow", graphcut::findSeam});
        a.push_back({"graphcut", "Graph Cut", "Minimum-energy seam using Edmonds-Karp max-flow", graphCutFinder });
        return a;
    }();
    return list;
}

const Algorithm* findAlgorithm(const std::string& id) {
    for (const Algorithm& a : algorithmList())
        if (a.id == id) return &a;
    return nullptr;
}

// RGBA bytes from the browser -> our RGB Image (alpha dropped, like loadImage).
Image fromRGBA(const std::vector<std::uint8_t>& rgba, int width, int height) {
    Image image = makeImage(height, width, 3);
    for (std::size_t p = 0, n = static_cast<std::size_t>(width) * static_cast<std::size_t>(height); p < n; ++p)
        for (std::size_t k = 0; k < 3; ++k) image.pixels[p * 3 + k] = rgba[p * 4 + k];
    return image;
}

std::vector<std::uint8_t> toRGBA(const Image& image) {
    const std::size_t n = static_cast<std::size_t>(image.rows) * static_cast<std::size_t>(image.cols);
    std::vector<std::uint8_t> out(n * 4, 255);
    for (std::size_t p = 0; p < n; ++p)
        for (std::size_t k = 0; k < 3; ++k)
            out[p * 4 + k] = image.pixels[p * static_cast<std::size_t>(image.channels) + (image.channels == 1 ? 0 : k)];
    return out;
}

// A copy of the bytes as a JS Uint8ClampedArray (ready for new ImageData).
val toJS(const std::vector<std::uint8_t>& bytes) {
    return val::global("Uint8ClampedArray").new_(emscripten::typed_memory_view(bytes.size(), bytes.data()));
}

val errorResult(const std::string& message) {
    val r = val::object();
    r.set("ok", false);
    r.set("error", message);
    return r;
}

Orientation toOrientation(int o) { return o == 1 ? Orientation::Horizontal : Orientation::Vertical; }

}  // namespace

class Carver {
public:
    Carver(const val& rgba, int width, int height) {
        if (width < 1 || height < 1) throw std::invalid_argument("picture must be at least 1 x 1");
        const std::vector<std::uint8_t> bytes = emscripten::convertJSArrayToNumberVector<std::uint8_t>(rgba);
        if (bytes.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4)
            throw std::invalid_argument("RGBA data does not match width x height");
        original_ = fromRGBA(bytes, width, height);
        current_ = original_;
        // Each surviving pixel's position in the original picture, kept in step
        // with the picture so removed seams can be drawn on the original (7a).
        rowOf_.rows = colOf_.rows = height;
        rowOf_.cols = colOf_.cols = width;
        rowOf_.values.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        colOf_.values.resize(rowOf_.values.size());
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c) {
                rowOf_.at(r, c) = r;
                colOf_.at(r, c) = c;
            }
        removedBy_.assign(rowOf_.values.size(), 0);
    }

    int width() const { return current_.cols; }
    int height() const { return current_.rows; }
    int originalWidth() const { return original_.cols; }
    int originalHeight() const { return original_.rows; }
    int seamsRemoved() const { return seamsRemoved_; }
    bool hasPending() const { return hasPending_; }

    // Energy of the current picture, then a seam from the chosen algorithm.
    // The seam is checked exactly like carve() does and kept as "pending".
    val findSeam(int orientation, const std::string& algorithm) {
        try {
            const Algorithm* a = findAlgorithm(algorithm);
            if (!a) return errorResult("unknown algorithm '" + algorithm + "'");
            const Orientation o = toOrientation(orientation);
            if ((o == Orientation::Vertical && current_.cols < 2) || (o == Orientation::Horizontal && current_.rows < 2))
                return errorResult("the picture is already 1 pixel wide/high in that direction");

            Clock::time_point t = Clock::now();
            const EnergyMap energy = sobelEnergy(current_);
            const double energyMs = msSince(t);

            t = Clock::now();
            Seam seam = a->finder(energy, o);
            const double findMs = msSince(t);

            if (seam.orientation != o) return errorResult("the finder returned a seam in the wrong direction");
            validateSeam(seam, energy.rows, energy.cols);
            validateSeamEnergy(seam, energy);

            pending_ = std::move(seam);
            hasPending_ = true;

            val r = val::object();
            r.set("ok", true);
            r.set("energy", pending_.energy);
            r.set("energyMs", energyMs);
            r.set("findMs", findMs);
            return r;
        } catch (const std::exception& e) {
            return errorResult(e.what());
        }
    }

    // The current picture with the pending seam painted (red / blue).
    val pendingRGBA() const {
        std::vector<std::uint8_t> rgba = toRGBA(current_);
        if (hasPending_) {
            const bool vertical = pending_.orientation == Orientation::Vertical;
            for (const auto& [r, c] : seamPixels(pending_)) {
                const std::size_t p = (static_cast<std::size_t>(r) * static_cast<std::size_t>(current_.cols) + static_cast<std::size_t>(c)) * 4;
                rgba[p] = vertical ? 255 : 30;
                rgba[p + 1] = vertical ? 30 : 110;
                rgba[p + 2] = vertical ? 30 : 255;
            }
        }
        return toJS(rgba);
    }

    // Removes the pending seam from the picture (and from the position maps).
    val removePending() {
        try {
            if (!hasPending_) return errorResult("no seam to remove; call findSeam first");
            const Clock::time_point t = Clock::now();
            const std::uint8_t mark = pending_.orientation == Orientation::Vertical ? 1 : 2;
            for (const auto& [r, c] : seamPixels(pending_)) {
                const std::size_t orig = static_cast<std::size_t>(rowOf_.at(r, c)) * static_cast<std::size_t>(original_.cols) +
                                         static_cast<std::size_t>(colOf_.at(r, c));
                removedBy_[orig] = mark;
            }
            current_ = removeSeam(current_, pending_);
            const double removeMs = msSince(t);
            rowOf_ = removeSeam(rowOf_, pending_);
            colOf_ = removeSeam(colOf_, pending_);
            hasPending_ = false;
            ++seamsRemoved_;
            val r = val::object();
            r.set("ok", true);
            r.set("removeMs", removeMs);
            return r;
        } catch (const std::exception& e) {
            return errorResult(e.what());
        }
    }

    val currentRGBA() const { return toJS(toRGBA(current_)); }

    // Energy map of the current picture as a grey picture (brighter = higher
    // energy). Scaled to the 99th percentile so a few very strong edges do not
    // make everything else black.
    val energyRGBA() const {
        const EnergyMap e = sobelEnergy(current_);
        std::vector<double> sorted = e.values;
        const std::size_t k = std::min(sorted.size() - 1, static_cast<std::size_t>(0.99 * static_cast<double>(sorted.size())));
        std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(k), sorted.end());
        const double top = std::max(1e-9, sorted[k]);
        std::vector<std::uint8_t> rgba(e.values.size() * 4, 255);
        for (std::size_t p = 0; p < e.values.size(); ++p) {
            const double v = std::min(1.0, e.values[p] / top);
            const auto g = static_cast<std::uint8_t>(std::lround(255.0 * std::sqrt(v)));
            rgba[p * 4] = rgba[p * 4 + 1] = rgba[p * 4 + 2] = g;
        }
        return toJS(rgba);
    }

    // The ORIGINAL picture with every removed pixel painted: vertical seams red,
    // horizontal seams blue (bonus 7a, all seams visualised).
    val seamsOverlayRGBA() const {
        std::vector<std::uint8_t> rgba = toRGBA(original_);
        for (std::size_t p = 0; p < removedBy_.size(); ++p) {
            if (removedBy_[p] == 0) continue;
            const bool vertical = removedBy_[p] == 1;
            rgba[p * 4] = vertical ? 235 : 40;
            rgba[p * 4 + 1] = vertical ? 40 : 110;
            rgba[p * 4 + 2] = vertical ? 40 : 245;
        }
        return toJS(rgba);
    }

private:
    Image original_, current_;
    EnergyMap rowOf_, colOf_;
    std::vector<std::uint8_t> removedBy_;  // 0 = kept, 1 = vertical seam, 2 = horizontal seam
    Seam pending_;
    bool hasPending_ = false;
    int seamsRemoved_ = 0;
};

// Available algorithms, in order of preference: [{id, label, note}, ...]
val algorithms() {
    val list = val::array();
    for (const Algorithm& a : algorithmList()) {
        val item = val::object();
        item.set("id", a.id);
        item.set("label", a.label);
        item.set("note", a.note);
        list.call<void>("push", item);
    }
    return list;
}

// One whole resize with the shared carve() loop, for the comparison panel.
// order: 0 width first, 1 height first, 2 alternate.
val compare(const val& rgba, int width, int height, int targetWidth, int targetHeight,
            const std::string& algorithm, int order) {
    try {
        const Algorithm* a = findAlgorithm(algorithm);
        if (!a) return errorResult("unknown algorithm '" + algorithm + "'");
        const std::vector<std::uint8_t> bytes = emscripten::convertJSArrayToNumberVector<std::uint8_t>(rgba);
        const Image image = fromRGBA(bytes, width, height);
        const CarveOrder o = order == 1 ? CarveOrder::HeightFirst : order == 2 ? CarveOrder::Alternate : CarveOrder::WidthFirst;
        const CarveResult result = carve(image, targetHeight, targetWidth, a->finder, o);
        double total = 0.0;
        for (const Seam& s : result.seams) total += s.energy;
        val r = val::object();
        r.set("ok", true);
        r.set("rgba", toJS(toRGBA(result.image)));
        r.set("width", result.image.cols);
        r.set("height", result.image.rows);
        r.set("seams", static_cast<int>(result.seams.size()));
        r.set("totalSeamEnergy", total);
        r.set("energyMs", result.timings.energy * 1000.0);
        r.set("findMs", result.timings.findSeam * 1000.0);
        r.set("removeMs", result.timings.remove * 1000.0);
        r.set("totalMs", result.timings.total * 1000.0);
        return r;
    } catch (const std::exception& e) {
        return errorResult(e.what());
    }
}

EMSCRIPTEN_BINDINGS(seam_carving_web) {
    emscripten::class_<Carver>("Carver")
        .constructor<const val&, int, int>()
        .function("width", &Carver::width)
        .function("height", &Carver::height)
        .function("originalWidth", &Carver::originalWidth)
        .function("originalHeight", &Carver::originalHeight)
        .function("seamsRemoved", &Carver::seamsRemoved)
        .function("hasPending", &Carver::hasPending)
        .function("findSeam", &Carver::findSeam)
        .function("pendingRGBA", &Carver::pendingRGBA)
        .function("removePending", &Carver::removePending)
        .function("currentRGBA", &Carver::currentRGBA)
        .function("energyRGBA", &Carver::energyRGBA)
        .function("seamsOverlayRGBA", &Carver::seamsOverlayRGBA);
    emscripten::function("algorithms", &algorithms);
    emscripten::function("compare", &compare);
}
