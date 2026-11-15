// ============================================================================
// main.cpp  --  seam_carver: command-line demo of the whole pipeline
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation: demo application).
//
//   seam_carver INPUT OUTPUT --width W --height H [--algorithm NAME] [options]
//   seam_carver --list
//   seam_carver --bench-sobel INPUT [--repeat N]
//
// Sizes are pixels ("320") or a percentage of the input ("70%").
// Run without arguments for the full help text.
//
// Only algorithms that are actually compiled into this build are offered.
// Each one is detected by its header: when a teammate's finder (DP, greedy,
// graph cut) is merged, it appears here automatically; nothing unfinished is
// ever presented as working.
// ============================================================================
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "carver.hpp"
#include "energy.hpp"
#include "image_utils.hpp"
#include "interfaces.hpp"
#include "seam_ops.hpp"

#if __has_include("dynamic_programming.hpp")
#include "dynamic_programming.hpp"  // YN: namespace dp { Seam findSeam(...); }
#define HAVE_DP 1
#endif
#if __has_include("greedy.hpp")
#include "greedy.hpp"  // HM: namespace greedy { Seam findSeam(...); }
#define HAVE_GREEDY 1
#endif
#if __has_include("graph_cut.hpp")
#include "graph_cut.hpp"  // KO: namespace graphcut { Seam findSeam(...); }
#define HAVE_GRAPH_CUT 1
#elif __has_include("seam_extraction.hpp")
#include "seam_extraction.hpp"  // KO's current API (vertical seams on a Grid)
#define HAVE_GRAPH_CUT_EXTRACTION 1
#endif

namespace {

struct Algorithm {
    std::string name;
    SeamFinder finder;
    std::string description;
};

#ifdef HAVE_GRAPH_CUT_EXTRACTION
// Temporary bridge until KO's graphcut::findSeam(EnergyMap, Orientation) exists:
// converts the energy map to WN/KO's Grid type, finds a vertical seam with
// graphcut::findVerticalGraphCutSeam, and handles horizontal seams by
// transposing (as the brief describes). The seam energy is summed here.
Seam graphCutViaExtraction(const EnergyMap& energy, Orientation orientation) {
    const EnergyMap map = (orientation == Orientation::Vertical) ? energy : transpose(energy);
    graphcut::Grid grid(static_cast<std::size_t>(map.rows), std::vector<double>(static_cast<std::size_t>(map.cols)));
    for (int r = 0; r < map.rows; ++r)
        for (int c = 0; c < map.cols; ++c) grid[r][c] = map.at(r, c);
    Seam seam;
    seam.orientation = orientation;
    seam.indices = graphcut::findVerticalGraphCutSeam(grid);
    seam.energy = seamEnergy(energy, seam);
    return seam;
}
#endif

// In order of preference: the first one is the default (fastest optimal method first).
std::vector<Algorithm> availableAlgorithms() {
    std::vector<Algorithm> algorithms;
#ifdef HAVE_DP
    algorithms.push_back({"dp", dp::findSeam, "dynamic programming (YN), optimal seam, Theta(rows*cols) per seam"});
#endif
#ifdef HAVE_GREEDY
    algorithms.push_back({"greedy", greedy::findSeam, "greedy (HM), cheapest neighbour row by row, not always optimal"});
#endif
#ifdef HAVE_GRAPH_CUT
    algorithms.push_back({"graphcut", graphcut::findSeam,
                          "graph cut (WN + KO), min s-t cut via Edmonds-Karp; very slow on large images"});
#elif defined(HAVE_GRAPH_CUT_EXTRACTION)
    algorithms.push_back({"graphcut", graphCutViaExtraction,
                          "graph cut (WN + KO via seam_extraction), min s-t cut via Edmonds-Karp; very slow on large images"});
#endif
    return algorithms;
}

const Algorithm* findAlgorithm(const std::vector<Algorithm>& algorithms, const std::string& name) {
    for (const Algorithm& a : algorithms)
        if (a.name == name) return &a;
    return nullptr;
}

void printUsage() {
    std::cout <<
        "seam_carver - content-aware image resizing (CSD3131 Assignment 2, Team T11)\n\n"
        "Usage:\n"
        "  seam_carver INPUT OUTPUT --width W --height H [options]\n"
        "  seam_carver --list                         show the algorithms in this build\n"
        "  seam_carver --bench-sobel INPUT [--repeat N]  time the three Sobel versions (item 7)\n\n"
        "Sizes: pixels (320) or percent of the input (70%). Omit one to keep it.\n"
        "Options:\n"
        "  --algorithm NAME   dp | greedy | graphcut (default: the first one available)\n"
        "  --order ORDER      width (default) | height | alternate\n"
        "  --energy FILE      save the energy map of the input as a picture\n"
        "  --seams FILE       save the input with every removed seam drawn on it\n"
        "  --frames DIR       save every step, with the seam about to go in red\n"
        "Formats: read PNG/JPEG/BMP/TGA/GIF/PNM; write .png .jpg .bmp .tga\n";
}

int parseSize(const std::string& text, int current, const char* name) {
    try {
        std::size_t used = 0;
        if (!text.empty() && text.back() == '%') {
            const double percent = std::stod(text.substr(0, text.size() - 1), &used);
            if (used != text.size() - 1 || percent <= 0.0) throw std::invalid_argument("");
            return std::max(1, static_cast<int>(current * percent / 100.0 + 0.5));
        }
        const int value = std::stoi(text, &used);
        if (used != text.size()) throw std::invalid_argument("");
        return value;
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string("bad ") + name + " '" + text +
                                    "' (use pixels such as 320 or a percentage such as 70%)");
    }
}

// Energy map as a grey picture: 0 -> black, the largest energy -> white.
Image energyPicture(const EnergyMap& energy) {
    Image out = makeImage(energy.rows, energy.cols, 1);
    const double top = *std::max_element(energy.values.begin(), energy.values.end());
    for (std::size_t i = 0; i < energy.values.size(); ++i)
        out.pixels[i] = static_cast<std::uint8_t>(top > 0.0 ? 255.0 * energy.values[i] / top + 0.5 : 0.0);
    return out;
}

double ms(double seconds) { return 1000.0 * seconds; }

int listAlgorithms() {
    const auto algorithms = availableAlgorithms();
    if (algorithms.empty()) {
        std::cout << "No seam-finding algorithm is compiled into this build yet.\n"
                     "They arrive when merged: DP (YN), greedy (HM), graph cut (KO + WN).\n";
        return 0;
    }
    for (const Algorithm& algorithm : algorithms)
        std::cout << "  " << std::left << std::setw(10) << algorithm.name << algorithm.description << "\n";
    return 0;
}

int benchSobel(const std::string& input, int repeat) {
    const Image image = loadImage(input);
    std::cout << input << ": " << image.rows << " x " << image.cols << ", " << repeat << " runs each\n";
    const EnergyMap reference = sobelEnergy(image);
    const auto measure = [&](const char* name, const std::function<EnergyMap()>& run) {
        double best = 1e300;
        EnergyMap result;
        for (int k = 0; k < repeat; ++k) {
            const auto t0 = std::chrono::steady_clock::now();
            result = run();
            best = std::min(best, std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
        }
        double maxDiff = 0.0;
        for (std::size_t i = 0; i < reference.values.size(); ++i)
            maxDiff = std::max(maxDiff, std::abs(result.values[i] - reference.values[i]));
        std::cout << "  " << std::left << std::setw(34) << name << std::right << std::fixed
                  << std::setprecision(3) << std::setw(9) << ms(best) << " ms (best)   max |diff| = "
                  << std::scientific << std::setprecision(1) << maxDiff << std::defaultfloat << "\n";
    };
    measure("direct 3x3 (sobelEnergy)", [&] { return sobelEnergy(image); });
    measure("separable 1x3 + 3x1", [&] { return sobelEnergySeparable(image); });
    for (int leaf : {64, 1024, 16384, 262144}) {
        const std::string name = "divide & conquer, leaf " + std::to_string(leaf) + " px";
        measure(name.c_str(), [&] { return sobelEnergyDivideAndConquer(image, leaf); });
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::vector<std::string> positional;
        std::map<std::string, std::string> options;
        bool list = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            } else if (arg == "--list") {
                list = true;
            } else if (arg.rfind("--", 0) == 0) {
                if (i + 1 >= argc) throw std::invalid_argument("missing value after " + arg);
                options[arg] = argv[++i];
            } else {
                positional.push_back(arg);
            }
        }
        const std::vector<std::string> known = {"--width", "--height", "--algorithm", "--order", "--energy",
                                                "--seams", "--frames", "--bench-sobel", "--repeat"};
        for (const auto& [key, value] : options)
            if (std::find(known.begin(), known.end(), key) == known.end())
                throw std::invalid_argument("unknown option " + key);

        if (list) return listAlgorithms();
        if (options.count("--bench-sobel"))
            return benchSobel(options["--bench-sobel"],
                              options.count("--repeat") ? std::max(1, std::stoi(options["--repeat"])) : 5);
        if (positional.size() != 2) {
            printUsage();
            return 1;
        }

        const auto algorithms = availableAlgorithms();
        if (algorithms.empty()) {
            std::cerr << "No seam-finding algorithm is compiled into this build yet, so nothing can be\n"
                         "resized. They arrive when merged: DP (YN), greedy (HM), graph cut (KO + WN).\n"
                         "Try --bench-sobel, or --list after merging.\n";
            return 2;
        }
        const std::string name = options.count("--algorithm") ? options["--algorithm"] : algorithms.front().name;
        const Algorithm* algorithm = findAlgorithm(algorithms, name);
        if (!algorithm) {
            std::cerr << "Algorithm '" << name << "' is not in this build. Available:\n";
            listAlgorithms();
            return 2;
        }
        // Check every output name before the (possibly slow) carving starts.
        checkOutputFormat(positional[1]);
        if (options.count("--energy")) checkOutputFormat(options["--energy"]);
        if (options.count("--seams")) checkOutputFormat(options["--seams"]);

        CarveOrder order = CarveOrder::WidthFirst;
        if (options.count("--order")) {
            const std::string o = options["--order"];
            if (o == "width") order = CarveOrder::WidthFirst;
            else if (o == "height") order = CarveOrder::HeightFirst;
            else if (o == "alternate") order = CarveOrder::Alternate;
            else throw std::invalid_argument("bad --order '" + o + "' (width, height or alternate)");
        }

        const Image input = loadImage(positional[0]);
        const int targetCols = options.count("--width") ? parseSize(options["--width"], input.cols, "--width") : input.cols;
        const int targetRows = options.count("--height") ? parseSize(options["--height"], input.rows, "--height") : input.rows;
        std::cout << "input      " << positional[0] << "  (" << input.rows << " x " << input.cols << ", "
                  << (input.channels == 3 ? "RGB" : "grey") << ")\n"
                  << "target     " << targetRows << " x " << targetCols << "\n"
                  << "algorithm  " << name << " - " << algorithm->description << "\n";
        if (name == "graphcut" && static_cast<long long>(input.rows) * input.cols > 20000 &&
            (input.rows - targetRows) + (input.cols - targetCols) > 0)
            std::cout << "note       graph cut needs a max-flow per seam (about 1.4 s per seam measured at\n"
                         "           121 x 178); try a small image or a few seams first\n";
        if (options.count("--energy")) saveImage(options["--energy"], energyPicture(sobelEnergy(input)));

        const bool keepFrames = options.count("--frames") > 0;
        const CarveResult result = carve(input, targetRows, targetCols, algorithm->finder, order, keepFrames);
        saveImage(positional[1], result.image);

        if (options.count("--seams")) {
            Image drawn = toRgb(input);
            const auto positions = seamsInOriginalCoordinates(input.rows, input.cols, result.seams);
            for (std::size_t s = 0; s < positions.size(); ++s) {
                const bool vertical = result.seams[s].orientation == Orientation::Vertical;
                for (const auto& [r, c] : positions[s]) {
                    drawn.at(r, c, 0) = vertical ? 230 : 30;
                    drawn.at(r, c, 1) = vertical ? 30 : 90;
                    drawn.at(r, c, 2) = vertical ? 30 : 230;
                }
            }
            saveImage(options["--seams"], drawn);
        }
        if (keepFrames) {
            const std::string dir = options["--frames"];
            for (std::size_t k = 0; k < result.frames.size(); ++k) {
                std::ostringstream file;
                file << dir << "/frame_" << std::setw(4) << std::setfill('0') << k << ".png";
                saveImage(file.str(), k < result.seams.size() ? highlightSeam(result.frames[k], result.seams[k])
                                                                : result.frames[k]);
            }
        }

        const std::size_t n = std::max<std::size_t>(1, result.seams.size());
        double seamTotal = 0.0;
        for (const Seam& seam : result.seams) seamTotal += seam.energy;
        std::cout << std::fixed << std::setprecision(2)
                  << "output     " << positional[1] << "  (" << result.image.rows << " x " << result.image.cols << ")\n"
                  << "seams      " << result.seams.size() << " removed, average seam energy "
                  << (result.seams.empty() ? 0.0 : seamTotal / static_cast<double>(n)) << "\n"
                  << "time       total " << ms(result.timings.total) << " ms = energy " << ms(result.timings.energy)
                  << " + find seam " << ms(result.timings.findSeam) << " (" << ms(result.timings.findSeam) / static_cast<double>(n)
                  << " per seam) + remove " << ms(result.timings.remove) << " (+ overhead)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
