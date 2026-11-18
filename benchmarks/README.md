# Benchmarks (Owner: HM)

The benchmark harness times the DP, Greedy and Graph Cut seam finders on the same images in the same way (`std::chrono::steady_clock` around the finder call only; see docs/interfaces.md §5) and records seam energies. RB uses the results for the comparison (item 9). Results go to `benchmarks/results/` (git-ignored). Copy the final tables into the report.

Graph cut is much slower than DP (one 100 × 100 seam takes about 0.5 s), so use small images (about 100–200 pixels wide) when carving many seams with it.

## Building

Build the project in Release first (`cmake --build build --config Release`). Then, until the
`benchmark` CMake target is added, compile the harness from the repository root:

```
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /std:c++17 /EHsc /O2 /MD /W4 /permissive- /utf-8 /I src\seam_carving benchmarks\benchmark.cpp build\Release\seam_carving.lib /Fo:build\ /Fe:build\benchmark.exe'
```

`/MD` must match the CMake-built library, otherwise linking fails with `LNK2038`.

## Running

```
.\build\benchmark.exe [IMAGE] [--repeat N] [--remove PERCENT] [--csv benchmarks\results]
```

- **Part 1** finds one vertical seam on random n × n energy maps (n = 25 … 400, fixed seed) with every
  finder in the build, and reports the median time and the seam energy relative to the DP optimum.
  Graph cut is skipped above 100 × 100.
- **Part 2** resizes one image (default `assets/images/synthetic_scene.png`) by `--remove` percent of its
  width with `carve()`, and reports the median energy / find-seam / remove / total time.

Only Part 1's seam energies compare the algorithms on identical input. In Part 2 the images differ after
the first seam, so the summed seam energy is only a rough indication.