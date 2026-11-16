// ============================================================================
// benchmark.cpp  --  benchmark harness: DP vs Greedy vs Graph Cut
// Owner: HM (Lee Hwee Min).
//
// Times every seam finder that is compiled into the build in the same way,
// so the comparison (item 9) is fair:
//
//   Part 1  Seam finding only, on random energy maps of growing size.
//           std::chrono::steady_clock around the finder call only
//           (docs/interfaces.md section 5). Each measurement is the MEDIAN of
//           --repeat runs, after one warm-up run that is not counted.
//           Also records the seam energy, so greedy can be compared with the
//           optimum (DP and graph cut must give the same, minimum energy).
//
//   Part 2  A full resize of one image with carve() (energy + find + remove),
//           using the timings that carve() itself measures. Median of --repeat.
//
// Usage:
//   benchmark [IMAGE] [--repeat N] [--remove PERCENT] [--csv DIR]
//     IMAGE      picture for part 2 (default: assets/images/synthetic_scene.png)
//     --repeat   runs per measurement (default 5)
//     --remove   percentage of the width to remove in part 2 (default 10)
//     --csv      also write part1.csv and part2.csv into DIR
//                (use benchmarks/results/, which is git-ignored)
//
// Build in Release: Debug timings are meaningless.
// ============================================================================
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "carver.hpp"
#include "image_utils.hpp"
#include "interfaces.hpp"

// Each finder is included only if its header exists, like src/main.cpp does,
// so the harness builds before every teammate's part is merged.
#if __has_include("dynamic_programming.hpp")
#include "dynamic_programming.hpp"  // FF: dp::findSeam
#define HAVE_DP 1
#endif
#if __has_include("greedy.hpp")
#include "greedy.hpp"  // HM: greedy::findSeam
#define HAVE_GREEDY 1
#endif
#if __has_include("graph_cut.hpp")
#include "graph_cut.hpp"  // KO: graphcut::findSeam
#define HAVE_GRAPH_CUT 1
#endif

namespace {

struct Algorithm {
    std::string name;
    SeamFinder finder;
    int maxSide;  // skip maps/images larger than this (graph cut is very slow)
};

std::vector<Algorithm> availableAlgorithms() {
    std::vector<Algorithm> algorithms;
#ifdef HAVE_DP
    algorithms.push_back({"dp", dp::findSeam, 1 << 30});
#endif
#ifdef HAVE_GREEDY
    algorithms.push_back({"greedy", greedy::findSeam, 1 << 30});
#endif
#ifdef HAVE_GRAPH_CUT
    algorithms.push_back({"graphcut", graphcut::findSeam, 100});
#endif
    return algorithms;
}

double medianOf(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const std::size_t mid = values.size() / 2;
    return values.size() % 2 == 1 ? values[mid] : (values[mid - 1] + values[mid]) / 2.0;
}

// Random energies in [0, 1000]. A fixed seed gives every run of the harness
// (and every algorithm) exactly the same maps.
EnergyMap randomEnergyMap(int rows, int cols, std::mt19937& rng) {
    std::uniform_real_distribution<double> value(0.0, 1000.0);
    EnergyMap map;
    map.rows = rows;
    map.cols = cols;
    map.values.resize(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols));
    for (double& v : map.values) v = value(rng);
    return map;
}

// Milliseconds for one call of `finder`, timed around the call only.
double timeFinderMs(const SeamFinder& finder, const EnergyMap& map, Seam& seamOut) {
    const auto start = std::chrono::steady_clock::now();
    seamOut = finder(map, Orientation::Vertical);
    const auto stop = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

// ---- Part 1: seam finding on random energy maps -----------------------------

void runPart1(const std::vector<Algorithm>& algorithms, int repeat, std::ostream* csv) {
    const std::vector<int> sizes = {25, 50, 100, 200, 400};

    std::cout << "\nPart 1: one vertical seam on random n x n energy maps (median of " << repeat
              << " runs, milliseconds)\n\n";
    std::cout << std::left << std::setw(10) << "size" << std::setw(10) << "algorithm" << std::right
              << std::setw(14) << "find (ms)" << std::setw(16) << "seam energy" << std::setw(14)
              << "vs optimum" << "\n";
    if (csv) *csv << "size,algorithm,median_ms,seam_energy,ratio_to_optimum\n";

    std::mt19937 rng(2026);
    for (int n : sizes) {
        const EnergyMap map = randomEnergyMap(n, n, rng);
        double optimum = -1.0;  // the DP energy, used to rate greedy

        for (const Algorithm& algorithm : algorithms) {
            if (n > algorithm.maxSide) {
                std::cout << std::left << std::setw(10) << (std::to_string(n) + "x" + std::to_string(n))
                          << std::setw(10) << algorithm.name << std::right << std::setw(14) << "skipped"
                          << "  (too slow above " << algorithm.maxSide << ")\n";
                continue;
            }
            Seam seam;
            timeFinderMs(algorithm.finder, map, seam);  // warm-up, not counted
            std::vector<double> times;
            for (int k = 0; k < repeat; ++k) times.push_back(timeFinderMs(algorithm.finder, map, seam));
            const double ms = medianOf(times);

            if (algorithm.name == "dp") optimum = seam.energy;
            const double ratio = optimum > 0.0 ? seam.energy / optimum : 0.0;

            std::cout << std::left << std::setw(10) << (std::to_string(n) + "x" + std::to_string(n))
                      << std::setw(10) << algorithm.name << std::right << std::fixed
                      << std::setprecision(4) << std::setw(14) << ms << std::setprecision(1)
                      << std::setw(16) << seam.energy;
            if (optimum > 0.0)
                std::cout << std::setprecision(3) << std::setw(13) << ratio << "x";
            std::cout << "\n";
            if (csv)
                *csv << n << "," << algorithm.name << "," << ms << "," << seam.energy << ","
                     << (optimum > 0.0 ? ratio : 0.0) << "\n";
        }
    }
}

// ---- Part 2: full resize of one image -----------------------------------------

void runPart2(const std::vector<Algorithm>& algorithms, const Image& image, int removePercent,
              int repeat, std::ostream* csv) {
    const int targetCols = std::max(1, image.cols - image.cols * removePercent / 100);

    std::cout << "\nPart 2: resize " << image.cols << " x " << image.rows << " -> " << targetCols
              << " x " << image.rows << " (" << (image.cols - targetCols)
              << " vertical seams, median of " << repeat << " runs, milliseconds)\n\n";
    std::cout << std::left << std::setw(10) << "algorithm" << std::right << std::setw(12) << "energy"
              << std::setw(12) << "find seam" << std::setw(12) << "remove" << std::setw(12)
              << "total" << std::setw(18) << "sum seam energy" << "\n";
    if (csv) *csv << "algorithm,energy_ms,find_ms,remove_ms,total_ms,sum_seam_energy\n";

    for (const Algorithm& algorithm : algorithms) {
        if (image.cols > algorithm.maxSide || image.rows > algorithm.maxSide) {
            std::cout << std::left << std::setw(10) << algorithm.name << std::right << std::setw(12)
                      << "skipped" << "  (image larger than " << algorithm.maxSide << " px)\n";
            continue;
        }
        std::vector<double> energyMs, findMs, removeMs, totalMs;
        double seamEnergySum = 0.0;
        for (int k = 0; k < repeat; ++k) {
            const CarveResult result = carve(image, image.rows, targetCols, algorithm.finder);
            energyMs.push_back(1000.0 * result.timings.energy);
            findMs.push_back(1000.0 * result.timings.findSeam);
            removeMs.push_back(1000.0 * result.timings.remove);
            totalMs.push_back(1000.0 * result.timings.total);
            seamEnergySum = 0.0;  // the same every run (deterministic finders)
            for (const Seam& seam : result.seams) seamEnergySum += seam.energy;
        }
        std::cout << std::left << std::setw(10) << algorithm.name << std::right << std::fixed
                  << std::setprecision(2) << std::setw(12) << medianOf(energyMs) << std::setw(12)
                  << medianOf(findMs) << std::setw(12) << medianOf(removeMs) << std::setw(12)
                  << medianOf(totalMs) << std::setprecision(1) << std::setw(18) << seamEnergySum
                  << "\n";
        if (csv)
            *csv << algorithm.name << "," << medianOf(energyMs) << "," << medianOf(findMs) << ","
                 << medianOf(removeMs) << "," << medianOf(totalMs) << "," << seamEnergySum << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string imagePath = "assets/images/synthetic_scene.png";
        std::string csvDir;
        int repeat = 5;
        int removePercent = 10;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--repeat" && i + 1 < argc) {
                repeat = std::stoi(argv[++i]);
            } else if (arg == "--remove" && i + 1 < argc) {
                removePercent = std::stoi(argv[++i]);
            } else if (arg == "--csv" && i + 1 < argc) {
                csvDir = argv[++i];
            } else if (!arg.empty() && arg[0] != '-') {
                imagePath = arg;
            } else {
                std::cerr << "Usage: benchmark [IMAGE] [--repeat N] [--remove PERCENT] [--csv DIR]\n";
                return 2;
            }
        }
        if (repeat < 1 || removePercent < 1 || removePercent > 90)
            throw std::invalid_argument("--repeat must be >= 1 and --remove between 1 and 90");

        const std::vector<Algorithm> algorithms = availableAlgorithms();
        if (algorithms.empty()) throw std::runtime_error("no seam finders are compiled into this build");

        std::cout << "Seam finder benchmark. Algorithms in this build:";
        for (const Algorithm& a : algorithms) std::cout << " " << a.name;
        std::cout << "\n";

        std::ofstream csv1, csv2;
        if (!csvDir.empty()) {
            std::filesystem::create_directories(csvDir);
            csv1.open(std::filesystem::path(csvDir) / "part1.csv");
            csv2.open(std::filesystem::path(csvDir) / "part2.csv");
        }

        runPart1(algorithms, repeat, csvDir.empty() ? nullptr : &csv1);
        runPart2(algorithms, loadImage(imagePath), removePercent, repeat,
                 csvDir.empty() ? nullptr : &csv2);

        if (!csvDir.empty()) std::cout << "\nCSV files written to " << csvDir << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}