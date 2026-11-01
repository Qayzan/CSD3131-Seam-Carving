# Benchmarks (Owner: HM)

The benchmark harness times the DP, Greedy and Graph Cut seam finders on the same images in the same way (`std::chrono::steady_clock` around the finder call only; see docs/interfaces.md §5) and records seam energies. RB uses the results for the comparison (item 9). Results go to `benchmarks/results/` (git-ignored). Copy the final tables into the report.

Graph cut is much slower than DP (one 100 × 100 seam takes about 0.5 s), so use small images (about 100–200 pixels wide) when carving many seams with it.
