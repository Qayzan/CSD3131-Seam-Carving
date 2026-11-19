// bench_improvements.cpp 
// Owner: RB (Rifqah Batrisya Binte Nazarudi).
//
// -- Section 5 evidence: DP vs greedy vs graph cut on REAL photographs and the
// improvement experiments of item 10 (threads, incremental energy). Uses only the team's headers
// (graph cut = Kervin's seam_extraction). Written with AI assistance.
//   bench_improvements seams   : seam-finder-only time + seam energy on Sobel maps, and first-seam quality of every image found
//   (images that are not in the images folder are skipped, so the program also runs with only the team's own images)
//   bench_improvements carve   : full resize through carve(): time split, avg seam energy, output vs DP
//   bench_improvements memory  : bytes per pixel of each method's working data
//   bench_improvements threads : Sobel on 1/2 threads, DP row-parallel, incremental energy update
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "carver.hpp"
#include "dynamic_programming.hpp"
#include "energy.hpp"
#include "greedy.hpp"
#include "image_utils.hpp"
#include "seam_extraction.hpp"
#include "seam_ops.hpp"
using Clock = std::chrono::steady_clock;
static double msSince(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now() - t).count(); }
static double median(std::vector<double> v) { std::sort(v.begin(), v.end()); return v[v.size() / 2]; }
static double timeMs(const std::function<void()>& f, int reps = 5) {
    f();  // warm-up
    std::vector<double> t;
    for (int i = 0; i < reps; ++i) { auto s = Clock::now(); f(); t.push_back(msSince(s)); }
    return median(t);
}
static Seam graphCut(const EnergyMap& energy, Orientation o) {
    const EnergyMap m = (o == Orientation::Vertical) ? energy : transpose(energy);
    graphcut::Grid g(m.rows, std::vector<double>(m.cols));
    for (int r = 0; r < m.rows; ++r) for (int c = 0; c < m.cols; ++c) g[r][c] = m.at(r, c);
    Seam s; s.orientation = o; s.indices = graphcut::findVerticalGraphCutSeam(g); s.energy = seamEnergy(energy, s);
    return s;
}
static Seam dpCompact(const EnergyMap& e, Orientation o) { return dp::findSeamCompact(e, o); }
static bool exists(const std::string& p) { return std::ifstream(p).good(); }
static Image crop(const Image& im, int rows, int cols) {
    Image o = makeImage(rows, cols, im.channels);
    for (int r = 0; r < rows; ++r) for (int c = 0; c < cols; ++c) for (int k = 0; k < im.channels; ++k) o.at(r, c, k) = im.at(r, c, k);
    return o;
}
static double meanAbsDiff(const Image& a, const Image& b) {
    if (a.pixels.size() != b.pixels.size()) return -1;
    double s = 0; for (size_t i = 0; i < a.pixels.size(); ++i) s += std::abs(int(a.pixels[i]) - int(b.pixels[i]));
    return s / a.pixels.size();
}

static void modeSeams(const std::string& dir) {
    std::printf("== seam finder only, vertical seam, Sobel map of building.jpg (top-left crops), median of 5 ==\n");
    std::printf("%-9s %10s %10s %10s %12s | %10s %10s\n", "size", "DP ms", "DPcompact", "greedy ms", "graphcut ms", "greedy/DP E", "GC/DP E");
    if (!exists(dir + "/building.jpg")) std::printf("(building.jpg not found in %s: scaling table skipped)\n", dir.c_str());
    else {
    const Image img = loadImage(dir + "/building.jpg");
    for (int n : {25, 50, 75, 100, 200, 400}) {
        const EnergyMap e = sobelEnergy(crop(img, n, n));
        Seam d = dp::findSeam(e, Orientation::Vertical), g = greedy::findSeam(e, Orientation::Vertical);
        double tdp = timeMs([&] { dp::findSeam(e, Orientation::Vertical); });
        double tdc = timeMs([&] { dp::findSeamCompact(e, Orientation::Vertical); });
        double tg = timeMs([&] { greedy::findSeam(e, Orientation::Vertical); });
        double tgc = -1, rgc = -1;
        if (n <= 100) { Seam c = graphCut(e, Orientation::Vertical); rgc = c.energy / d.energy; tgc = timeMs([&] { graphCut(e, Orientation::Vertical); }, n <= 75 ? 5 : 3); }
        std::printf("%3dx%-5d %10.4f %10.4f %10.4f %12.3f | %10.3f %10.4f\n", n, n, tdp, tdc, tg, tgc, g.energy / d.energy, rgc);
    }
    }
    std::printf("\n== first-seam quality on whole images (graph cut only on images of at most 15,000 pixels) ==\n");
    for (std::string f : {"synthetic_scene.png", "test_small.png", "building.jpg", "messi5.jpg", "fruits.jpg", "starry_night.jpg"}) {
        if (!exists(dir + "/" + f)) continue;
        const Image im = loadImage(dir + "/" + f); const EnergyMap e = sobelEnergy(im);
        for (Orientation o : {Orientation::Vertical, Orientation::Horizontal}) {
            Seam d = dp::findSeam(e, o), g = greedy::findSeam(e, o);
            std::printf("%-20s %-10s DP E=%9.1f  greedy E=%9.1f  ratio=%.2f", f.c_str(), toString(o), d.energy, g.energy, d.energy > 0 ? g.energy / d.energy : 0.0);
            if (im.rows * im.cols <= 15000) std::printf("  graph cut E=%9.1f", graphCut(e, o).energy);
            std::printf("\n");
        }
    }
}

// Where do the removed seams lie? Counts the removed pixels per ORIGINAL column.
static void printSpread(const Image& im, const CarveResult& r, const char* name) {
    const auto pos = seamsInOriginalCoordinates(im.rows, im.cols, r.seams);
    std::vector<long long> cols(im.cols, 0); long long total = 0;
    for (const auto& s : pos) for (const auto& rc : s) { ++cols[rc.second]; ++total; }
    int touched = 0; long long left30 = 0, run = 0; int lo = -1, hi = -1;
    for (int c = 0; c < im.cols; ++c) { if (cols[c]) ++touched; if (c < im.cols * 3 / 10) left30 += cols[c];
        run += cols[c]; if (lo < 0 && run >= 0.05 * total) lo = c; if (hi < 0 && run >= 0.95 * total) hi = c; }
    std::printf("    spread[%-8s] columns touched %d/%d | middle 90%% of removed pixels span %.0f%% of the width | %.0f%% lie in the left 30%%\n",
                name, touched, im.cols, 100.0 * (hi - lo) / im.cols, 100.0 * left30 / total);
}
static void modeCarve(const std::string& dir, const std::string& out) {
    struct Case { std::string file; double pct; bool withGC; };
    std::vector<Case> cases = {{"synthetic_scene.png", 0.9, false}, {"synthetic_scene.png", 0.6, false}, {"test_small.png", 0.8, true}, {"test_small.png", 0.6, true},
                               {"building.jpg", 0.8, false}, {"messi5.jpg", 0.8, false}};   // withGC only where the image is tiny
    std::filesystem::create_directories(out);
    struct F { const char* name; SeamFinder f; };
    std::vector<F> fs = {{"dp", dp::findSeam}, {"dp-compact", dpCompact}, {"greedy", greedy::findSeam}, {"graphcut", graphCut}};
    std::printf("%-20s %-10s %9s %9s | %8s %8s %8s %9s | %9s %s\n", "image (WxH -> W')", "algo", "avgSeamE", "totalE", "energy", "find", "remove", "TOTAL ms", "diffVsDP", "identical");
    for (auto& c : cases) {
        if (!exists(dir + "/" + c.file)) { std::printf("(%s not found: skipped)\n", c.file.c_str()); continue; }
        const Image im = loadImage(dir + "/" + c.file);
        const std::string tag = c.file.substr(0, c.file.find('.')) + std::to_string(int(c.pct * 100));   // e.g. synthetic_scene60
        const int tc = int(im.cols * c.pct);
        CarveResult ref;
        for (auto& f : fs) {
            if (std::string(f.name) == "graphcut" && !c.withGC) continue;
            CarveResult r = carve(im, im.rows, tc, f.f);
            if (std::string(f.name) != "graphcut") {   // stage times = median of 7 runs (graph cut takes seconds: one run)
                std::vector<double> E{r.timings.energy}, Fi{r.timings.findSeam}, Re{r.timings.remove}, To{r.timings.total};
                for (int k = 0; k < 6; ++k) { CarveResult q = carve(im, im.rows, tc, f.f); E.push_back(q.timings.energy); Fi.push_back(q.timings.findSeam); Re.push_back(q.timings.remove); To.push_back(q.timings.total); }
                r.timings.energy = median(E); r.timings.findSeam = median(Fi); r.timings.remove = median(Re); r.timings.total = median(To);
            }
            double tot = 0; for (auto& s : r.seams) tot += s.energy;
            if (std::string(f.name) == "dp") ref = r;
            double md = meanAbsDiff(r.image, ref.image);
            std::printf("%-20s %-10s %9.1f %9.0f | %8.1f %8.1f %8.1f %9.1f | %9.4f %s\n", (c.file + " " + std::to_string(im.cols) + "->" + std::to_string(tc)).c_str(), f.name, tot / r.seams.size(), tot,
                        1000 * r.timings.energy, 1000 * r.timings.findSeam, 1000 * r.timings.remove, 1000 * r.timings.total, md, r.image.pixels == ref.image.pixels ? "yes" : "no");
            if (std::string(f.name) == "dp" || std::string(f.name) == "greedy" || std::string(f.name) == "graphcut") printSpread(im, r, f.name);
            {
                saveImage(out + "/" + tag + "_" + f.name + ".png", r.image);
                if (std::string(f.name) == "dp" || std::string(f.name) == "greedy" || std::string(f.name) == "graphcut") {
                    Image drawn = toRgb(im);
                    auto pos = seamsInOriginalCoordinates(im.rows, im.cols, r.seams);
                    for (auto& s : pos) for (auto& [rr, cc] : s) { drawn.at(rr, cc, 0) = 230; drawn.at(rr, cc, 1) = 30; drawn.at(rr, cc, 2) = 30; }
                    saveImage(out + "/" + tag + "_" + f.name + "_seams.png", drawn);
                }
            }
        }
    }
}

static void modeMemory() {
    std::printf("== working memory per pixel (bytes), computed from the real structs ==\n");
    for (int n : {50, 100}) {
        graphcut::Grid g(n, std::vector<double>(n, 1.0));
        graphcut::SeamGraph sg = graphcut::buildVerticalSeamGraph(g);
        size_t edgeBytes = sg.net.edges().size() * sizeof(graphcut::Edge), adjBytes = 0;
        for (int u = 0; u < sg.net.nodeCount(); ++u) adjBytes += sg.net.edgesFrom(u).size() * sizeof(int);
        std::printf("graph cut %dx%d: %zu edge entries, %zu B edges + %zu B adjacency = %.1f B/pixel (excl. vector slack)\n", n, n, sg.net.edges().size(), edgeBytes, adjBytes, double(edgeBytes + adjBytes) / (n * n));
    }
    std::printf("DP full table: %zu B/pixel (double M + int8 back);  DP compact: %zu B/pixel (int8 back) + 2 rows of double;  greedy: 0 B/pixel (seam only)\n", sizeof(double) + sizeof(std::int8_t), sizeof(std::int8_t));
    const long long px = 1920LL * 2880;
    std::printf("1920x2880: DP full %.1f MB, DP compact %.1f MB (+%.0f KB rows)\n", px * 9 / 1e6, px * 1 / 1e6, 2.0 * 2880 * 8 / 1e3);
}

// ---- prototypes for item 10 (bench only; not in the team's code) -------------------------------------
struct Tables { std::vector<int> up, down, left, right; };
static Tables makeTables(int R, int C) { Tables t; t.up.resize(R); t.down.resize(R); t.left.resize(C); t.right.resize(C);
    for (int r = 0; r < R; ++r) { t.up[r] = reflect101(r - 1, R); t.down[r] = reflect101(r + 1, R); }
    for (int c = 0; c < C; ++c) { t.left[c] = reflect101(c - 1, C); t.right[c] = reflect101(c + 1, C); } return t; }
static inline double sobelPixel(const std::vector<double>& g, int C, const Tables& t, int r, int c) {
    const int ru = t.up[r], rd = t.down[r], cl = t.left[c], cr = t.right[c];
    auto G = [&](int a, int b) { return g[size_t(a) * C + b]; };
    double gx = (G(ru, cr) - G(ru, cl)) + 2.0 * (G(r, cr) - G(r, cl)) + (G(rd, cr) - G(rd, cl));
    double gy = (G(rd, cl) - G(ru, cl)) + 2.0 * (G(rd, c) - G(ru, c)) + (G(rd, cr) - G(ru, cr));
    return std::fabs(gx) + std::fabs(gy);
}
// Filtering step only (grey picture already computed); rows split into `threads` bands.
static void sobelFilterBands(const std::vector<double>& g, int R, int C, const Tables& t, EnergyMap& out, int threads) {
    auto band = [&](int r0, int r1) { for (int r = r0; r < r1; ++r) for (int c = 0; c < C; ++c) out.at(r, c) = sobelPixel(g, C, t, r, c); };
    if (threads == 1) { band(0, R); return; }
    std::vector<std::thread> th; for (int k = 0; k < threads; ++k) th.emplace_back(band, R * k / threads, R * (k + 1) / threads);
    for (auto& x : th) x.join();
}
// Incremental state: grey picture + energy map of the CURRENT image.
struct Inc { int R, C; std::vector<double> g; EnergyMap E; };
static void incRemove(Inc& s, const Seam& seam) {   // remove a vertical seam and repair only the band near it
    const int R = s.R, C = s.C - 1;
    std::vector<double> g2(size_t(R) * C); EnergyMap E2; E2.rows = R; E2.cols = C; E2.values.resize(size_t(R) * C);
    for (int r = 0; r < R; ++r) { const int k = seam.indices[r];
        std::copy(s.g.begin() + size_t(r) * s.C, s.g.begin() + size_t(r) * s.C + k, g2.begin() + size_t(r) * C);
        std::copy(s.g.begin() + size_t(r) * s.C + k + 1, s.g.begin() + size_t(r + 1) * s.C, g2.begin() + size_t(r) * C + k);
        std::copy(s.E.values.begin() + size_t(r) * s.C, s.E.values.begin() + size_t(r) * s.C + k, E2.values.begin() + size_t(r) * C);
        std::copy(s.E.values.begin() + size_t(r) * s.C + k + 1, s.E.values.begin() + size_t(r + 1) * s.C, E2.values.begin() + size_t(r) * C + k); }
    const Tables t = makeTables(R, C);
    for (int r = 0; r < R; ++r) { int lo = seam.indices[r], hi = lo;
        for (int d = -1; d <= 1; ++d) if (r + d >= 0 && r + d < R) { lo = std::min(lo, seam.indices[r + d]); hi = std::max(hi, seam.indices[r + d]); }
        lo = std::max(0, lo - 2); hi = std::min(C - 1, hi + 1);
        for (int c = lo; c <= hi; ++c) E2.at(r, c) = sobelPixel(g2, C, t, r, c);
        E2.at(r, 0) = sobelPixel(g2, C, t, r, 0); E2.at(r, C - 1) = sobelPixel(g2, C, t, r, C - 1); }  // borders: reflect rule changes
    s.g.swap(g2); s.E = std::move(E2); s.C = C;
}
static void modeThreads(const std::string& dir) {
    std::printf("hardware threads: %u\n", std::thread::hardware_concurrency());
    if (!exists(dir + "/building.jpg")) { std::printf("building.jpg not found in %s: threads mode needs it\n", dir.c_str()); return; }
    const Image im = loadImage(dir + "/building.jpg");
    const EnergyMap ref = sobelEnergy(im); const int R = im.rows, C = im.cols;
    std::vector<double> g = toGrayscale(im); const Tables tb = makeTables(R, C);
    std::printf("Sobel %dx%d\n  team sobelEnergy (grey + filter, 1 thread): %.2f ms\n  grey conversion alone: %.2f ms\n", R, C, timeMs([&] { sobelEnergy(im); }, 15), timeMs([&] { toGrayscale(im); }, 15));
    for (int t : {1, 2}) { EnergyMap o = ref; sobelFilterBands(g, R, C, tb, o, t);
        std::printf("  filter step only, %d thread(s): %.2f ms (output identical to team's: %s)\n", t, timeMs([&] { sobelFilterBands(g, R, C, tb, o, t); }, 15), o.values == ref.values ? "yes" : "NO"); }
    // Incremental energy update vs. recomputing everything, over 150 real DP seams.
    Image cur = im; Inc st{R, C, g, ref}; int bad = 0; double tFull = 0, tInc = 0; const int K = 150;
    for (int k = 0; k < K; ++k) {
        Seam s = dp::findSeam(st.E, Orientation::Vertical);
        cur = removeSeam(cur, s);
        auto t0 = Clock::now(); EnergyMap full = sobelEnergy(cur); tFull += msSince(t0);
        t0 = Clock::now(); incRemove(st, s); tInc += msSince(t0);
        if (st.E.values != full.values) ++bad;
    }
    std::printf("incremental energy: %d real seams, %d maps differ from a full recompute; per seam: full %.3f ms vs incremental (incl. its own copy) %.3f ms = %.1fx\n", K, bad, tFull / K, tInc / K, tFull / tInc);
    // DP: parallelising across the cells of a row.
    const EnergyMap e = ref;
    double tSeq = timeMs([&] { dp::findSeamCompact(e, Orientation::Vertical); });
    auto rowPar = [&] { std::vector<double> a(e.values.begin(), e.values.begin() + C), b(C);
        for (int i = 1; i < R; ++i) { auto work = [&](int j0, int j1) { for (int j = j0; j < j1; ++j) { double m = a[j]; if (j > 0) m = std::min(m, a[j - 1]); if (j + 1 < C) m = std::min(m, a[j + 1]); b[j] = e.at(i, j) + m; } };
            std::thread th(work, 0, C / 2); work(C / 2, C); th.join(); std::swap(a, b); } };
    std::printf("DP vertical seam %dx%d: sequential %.3f ms vs row-parallel, 2 threads, one thread launched per row %.3f ms\n", R, C, tSeq, timeMs(rowPar, 5));
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "seams", dir = argc > 2 ? argv[2] : "assets/images", out = argc > 3 ? argv[3] : "benchmarks/results";
    if (mode == "seams") modeSeams(dir); else if (mode == "carve") modeCarve(dir, out);
    else if (mode == "memory") modeMemory(); else if (mode == "threads") modeThreads(dir);
    else { std::puts("usage: bench_improvements seams|carve|memory|threads [imagesdir] [outdir]"); return 1; }
}