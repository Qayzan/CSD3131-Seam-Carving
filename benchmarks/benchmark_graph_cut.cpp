// ============================================================================
// benchmark_graph_cut.cpp -- Graph Cut runtime benchmarking
// Owner: Kervin (Graph Cut Part 2).
//
// Measures the time taken for graph construction, Edmonds-Karp maximum flow,
// and seam extraction using synthetic energy maps of different sizes.
// ============================================================================

#include "seam_extraction.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::steady_clock;

double elapsedMs(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void benchmark(int size) {
    // Use a fixed seed so results are reproducible.
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> distribution(1, 255);

    graphcut::Grid energy(size, std::vector<double>(size));

    for (int row = 0; row < size; ++row) {
        for (int col = 0; col < size; ++col) {
            energy[row][col] = distribution(rng);
        }
    }

    // Measure each stage separately.
    auto start = Clock::now();
    auto graph = graphcut::buildVerticalSeamGraph(energy);
    auto afterBuild = Clock::now();

    graph.net.maxFlow(graph.source, graph.sink);
    auto afterFlow = Clock::now();

    auto seam = graphcut::extractVerticalSeam(graph);
    auto afterExtraction = Clock::now();

    double buildMs = elapsedMs(start, afterBuild);
    double flowMs = elapsedMs(afterBuild, afterFlow);
    double extractMs = elapsedMs(afterFlow, afterExtraction);
    double totalMs = elapsedMs(start, afterExtraction);

    std::cout << std::fixed << std::setprecision(3)
              << size << "x" << size
              << " | Build: " << buildMs << " ms"
              << " | Max-flow: " << flowMs << " ms"
              << " | Extraction: " << extractMs << " ms"
              << " | Total: " << totalMs << " ms"
              << " | Seam length: " << seam.size()
              << '\n';
}

int main() {
    for (int size : {10, 20, 30, 40}) {
        benchmark(size);
    }

    return 0;
}
