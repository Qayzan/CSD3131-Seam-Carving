# Architecture and algorithm design

Status: **design proposal**. Sections marked *decision* are proposals the team can still change before code depends on them. Owners: architecture and §2.1/§2.6–2.8 shared foundation (FF, item 1); §2.2–2.5 and §2.9–2.10 DP (YN, items 2, 6, 8); §3 Graph Cut (WN part 1, KO part 2, item 4). §2.11 is input for RB's improvements (item 10).

---

## 1. Architecture

### 1.1 Stack and why

| Choice | Reason |
|---|---|
| **C++17** | Fast enough for graph cut on real images (max-flow on hundreds of thousands of nodes). Explicit loops make every step of the algorithms visible for the report and the quiz. |
| **CMake** | One build description for Visual Studio (Windows) and g++ (Linux, CI). New source and test files are picked up automatically. |
| **stb_image / stb_image_write** (*proposal*, FF decides) | Image I/O from two public-domain header files in `third_party/stb/`, so nobody has to install a library. The Sobel filter is written by hand, which also supports the divide-and-conquer discussion (item 7). |
| **Plain test programs + CTest** | Each `tests/test_*.cpp` prints PASS/FAIL per check and returns 0 on success. No test framework to install. |
| **GitHub Actions** | Builds and tests on Linux (g++) and Windows (MSVC) on every pull request. This is the CI part of bonus item 7(c). |

The demo app (`src/main.cpp`) is a command-line program: input image, target size, method. A small UI could be added later for bonus 7(a)/(b). `carve` will already expose every intermediate step such a UI needs.

### 1.2 Module map and data flow

```
              loadImage()                                   (RGB, rows × cols × 3)
                         │
                         ▼
     ┌────────── carve(image, target, finder) ─────────────────┐
     │  loop until target size:                                │
     │    E    = sobelEnergy(image)              (EnergyMap)   │  ◄── dp::findSeam        (YN)
     │    seam = finder(E, orientation)          (Seam)        │  ◄── greedy::findSeam    (HM)
     │    validateSeam(seam, E.rows, E.cols)                   │  ◄── graphcut::findSeam  (KO, on
     │    image = removeSeam(image, seam)                      │        WN's graph + max-flow)
     │    (optionally record frame / seam for visualisation)   │
     └─────────────────────────────────────────────────────────┘
                         │
                         ▼
              CarveResult(image, seams, timings, frames)  →  demo app, examples/, benchmarks/
```

The three algorithms differ **only** in `finder`. Everything else is shared, so their comparison (item 9) is fair: the same energy function, the same removal code, the same timing method.

### 1.3 Conventions (summary — full contract in [interfaces.md](interfaces.md))

- `[row][col]`, row 0 at the top. Colour images are RGB.
- Energy: `double`, `rows × cols`, non-negative, stored row by row.
- Vertical seam: `std::vector<int>` of length `rows`. Entry `i` is the column removed from row `i`, and neighbouring entries differ by at most 1.
- Horizontal seam: length `cols`. Entry `j` is the row removed from column `j`, and neighbouring entries differ by at most 1.
- The brief uses 1-based `img[1…m, 1…n]`. The code uses 0-based indices. The report will say so once.

---

## 2. Dynamic Programming seam carving

### 2.1 Energy map (R1a, item 1) — *decision*

Use the **e1 gradient-magnitude energy** of Avidan & Shamir (2007):

$$e(I) = \left|\frac{\partial I}{\partial x}\right| + \left|\frac{\partial I}{\partial y}\right|$$

- Convert RGB to greyscale **as `double`** first. This avoids 8-bit overflow and wrap-around: `200 − 250` must be −50, not 206.
- Derivatives come from the 3×3 **Sobel** operator. Sobel smooths across the perpendicular direction, so single-pixel noise counts less than with a plain neighbour difference.
- **Borders:** mirror the image at the edge (reflect without repeating the border pixel). Without it, border pixels would see a jump to zero and look like strong edges, so no seam would ever use the image border.
- Why this energy: it is cheap (O(HW)), it is the energy used in the original paper and in the brief, and it is high exactly where removing a pixel would be visible (edges, texture).
- Alternatives (documented for the report, not default): per-channel gradients summed over R, G, B (catches colour edges of equal brightness), and *forward energy* (Rubinstein et al., 2008). Forward energy measures the new edges a removal creates. The graph-cut question mentions it too.

### 2.2 Why dynamic programming applies

The problem: among all vertical seams (one pixel per row, neighbours in adjacent rows at most one column apart), find the one with the smallest total energy.

- **Brute force is hopeless.** Each step down has up to 3 choices, so there are up to n·3^(m−1) seams. Q2(a)(i) asks for the exponential lower bound; that proof is item 5 (HM). The test oracle below enumerates them only for tiny matrices.
- **Optimal substructure.** Take the cheapest seam that ends at pixel (i, j). Its part above row i is itself the cheapest seam that ends at whichever pixel (i−1, k) it passes through, with k ∈ {j−1, j, j+1}. If there were a cheaper seam ending at (i−1, k), we could swap it in and get a cheaper seam to (i, j), which is a contradiction. So optimal answers are built from optimal answers to smaller sub-problems.
- **Overlapping sub-problems.** The best seam to (i−1, k) is reused by up to three pixels below it: (i, k−1), (i, k), (i, k+1). Plain recursion would recompute it exponentially often. DP computes each of the m·n sub-problems **once** and stores it in a table: the course's time–memory trade-off.

### 2.3 Recurrence (R1b, items 2 and 6)

For energy matrix `E` (H rows, W columns), `M[i][j]` = minimum total energy of any seam from row 0 down to pixel (i, j):

```
Base case:   M[0][j] = E[0][j]                                     for 0 ≤ j < W
Recurrence:  M[i][j] = E[i][j] + min( M[i-1][j-1], M[i-1][j], M[i-1][j+1] )   for 1 ≤ i < H
             (only valid columns: out-of-range neighbours count as +∞)
Answer:      min_j M[H-1][j]
```

Alongside `M` we keep a **back-pointer table** `back[i][j] ∈ {−1, 0, +1}`, which says which predecessor gave the minimum. This makes backtracking trivial and lets us check the result.

**Tie-breaking (*decision*): deterministic, "leftmost wins".** When predecessors tie, choose the smallest column (j−1 before j before j+1). In the bottom row, choose the smallest j among equal minima. Comparing with strict `<` in that order gives exactly this rule, so the tests can assert exact seams, not just exact energies.

### 2.4 Algorithm (pseudocode)

```
FIND-VERTICAL-SEAM(E):                      ▷ E is H × W
 1  M ← copy of E;  back ← H × W table of zeros
 2  for i ← 1 to H − 1:
 3      for j ← 0 to W − 1:
 4          best ← M[i−1][j];  back[i][j] ← 0
 5          if j > 0     and M[i−1][j−1] ≤ best:  best ← M[i−1][j−1];  back[i][j] ← −1
 6          if j < W − 1 and M[i−1][j+1] < best:  best ← M[i−1][j+1];  back[i][j] ← +1
 7          M[i][j] ← E[i][j] + best
 8  seam[H−1] ← argmin_j M[H−1][j]           ▷ leftmost minimum
 9  for i ← H − 1 down to 1:
10      seam[i−1] ← seam[i] + back[i][seam[i]]
11  return seam, M[H−1][seam[H−1]]
```

Lines 5–6 implement the tie rule: `≤` lets j−1 win ties against j, and `<` stops j+1 from winning a tie. The loop over rows is inherently sequential, because row i needs row i−1 (the DP dependency). The cells inside a row are independent.

**Self-check built into the tests.** The returned energy `M[H−1][seam[H−1]]` must equal `Σ_i E[i][seam[i]]`, and the seam must be 8-connected.

### 2.5 Worked example

Energy matrix (4 × 5):

```
E =  3  1  4  1  5
     9  2  6  5  3
     5  8  1  9  7
     2  6  5  3  5
```

Cumulative table M (row by row: each entry = E + cheapest of the up-to-3 entries above it):

```
M =  3  1  4  1  5
    10  3  7  6  4
     8 11  4 13 11
    10 10  9  7 16
```

Row 1, column 2 shows the tie rule. Its predecessors are M[0][1] = 1, M[0][2] = 4 and M[0][3] = 1. Columns 1 and 3 tie, so the leftmost (column 1) wins: back[1][2] = −1 and M[1][2] = 6 + 1 = 7.

Backtracking starts at the smallest value in the bottom row: M[3][3] = 7. Following the back-pointers gives:

```
seam (column per row) = [1, 1, 2, 3]      energies 1 + 2 + 1 + 3 = 7
```

A brute-force check of all 95 valid seams of this matrix also gives a minimum of 7 at the same seam. This example becomes a unit test.

### 2.6 Horizontal seams (item 1) — *decision: transpose*

A horizontal seam in `E` is exactly a vertical seam in `Eᵀ`:

```
FIND-HORIZONTAL-SEAM(E):  rows, cost ← FIND-VERTICAL-SEAM(Eᵀ);  return rows   ▷ length W
```

- One implementation per algorithm serves both orientations, so there is less code to test and nothing can diverge between them.
- The transpose is one Θ(HW) copy. After it, the row-by-row loops read memory in order, which is cache-friendly.
- Coordinates come back as `rows[j]` for column `j` (the contract in §1.3). Tests use **non-square** images (e.g. 4 × 7 and 7 × 4), so mixing up H and W fails loudly.

### 2.7 Seam removal (R1c, item 1)

- **Vertical:** copy every row into a new image one column narrower, skipping the pixel at column `seam[i]`. One pass over the image. All other pixel values and their order are preserved.
- **Horizontal:** the same for columns (or remove from the transposed image and transpose back).
- **Tests:** new size is exactly one column (or row) smaller; every remaining pixel equals the original pixel at its shifted position; the seam pixels are gone; greyscale and RGB both work.

### 2.8 Repeated resizing (item 1) — *decision*

```
CARVE(image, target_h, target_w, finder):
  validate: 1 ≤ target_h ≤ H, 1 ≤ target_w ≤ W   (shrinking only; error otherwise)
  while width  > target_w:  E ← energy(image); s ← finder(E, VERTICAL);   image ← remove(image, s)
  while height > target_h:  E ← energy(image); s ← finder(E, HORIZONTAL); image ← remove(image, s)
```

- **The energy is recalculated after every removal**, as R1c requires. Removing a seam brings formerly separated pixels together, so their gradients change.
- **Order:** width first, then height. It is simple and reproducible. An `Alternate` option will interleave the two directions. The order that is *optimal* overall needs a second DP over (rows removed, columns removed): the "transport map" of Avidan & Shamir §3.3. We mention it in the report as an improvement and do not implement it by default (it costs much more time).
- **Enlargement** (seam insertion) is out of scope unless the team asks for it. The brief's Task 1 steps only cover removal.

### 2.9 Complexity (item 8, DP part)

Let the image be H × W, and remove k seams.

| Step | Time | Extra space | Notes |
|---|---|---|---|
| Energy (Sobel, 3×3) | Θ(HW) | Θ(HW) | constant work per pixel |
| DP table M | Θ(HW) | Θ(HW) doubles | 3 comparisons per cell |
| Back-pointers | Θ(HW) (inside the DP) | Θ(HW) bytes | 1 byte per cell (`int8_t`) |
| Backtracking | Θ(H) | Θ(H) | one step per row |
| Seam removal | Θ(HW) | Θ(HW) (new image) | copying dominates |
| **One seam total** | **Θ(HW)** | **Θ(HW)** | |
| k vertical seams | Θ(k·H·W) | Θ(HW) | width shrinks each time, so precisely Σ H·(W−t) |
| Horizontal seam | Θ(HW) | Θ(HW) | plus one Θ(HW) transpose copy |

### 2.10 Do we need the whole DP table? (input for item 10)

- Computing **only the minimum cost** needs just the previous row of M: **Θ(W) space** (two rolling rows).
- Recovering the **seam itself** needs to know, for every row, which predecessor was chosen. Options:
  1. Keep the full `double` table M and recompute the choices while backtracking: 8·H·W bytes.
  2. **Keep two rolling rows of M plus an `int8_t` back-pointer table**: 2·8·W + H·W bytes, about 8× less memory.
  3. Hirschberg-style divide & conquer: Θ(W) space, but about 2× the time and a lot more complexity. Mentioned in the report only.
- Even option 2 is still Θ(HW) in total, because the seam depends on choices in every row.

### 2.11 Would multithreading help? (input for item 10)

- **Across rows: no.** Row i depends on row i−1, so the DP is inherently sequential in i.
- **Within a row: yes in principle.** The W cells of a row are independent and could be split across threads (e.g. OpenMP `parallel for`). For typical image widths (hundreds to a few thousand pixels), starting threads for every row costs more than it saves; the compiler's SIMD vectorisation of the inner loop helps more.
- **Energy computation:** embarrassingly parallel. Every pixel's Sobel value is independent.
- **Graph cut:** max-flow augmentations change the residual graph that the next BFS reads, so Edmonds-Karp is hard to parallelise. Parallel push-relabel algorithms exist; mention them as an improvement.
- **Across seams: no.** Seam t+1 depends on the image after seam t is removed.
- Timings from the benchmark harness (HM) will back these arguments with measurements, not assumptions.

### 2.12 Visualisation hooks (for the demo and bonus 7a)

The DP finder and `carve` can optionally return: the energy map, the cumulative table M (normalised for display), the back-pointer path, seam coordinates, the image with the seam highlighted, the image after removal, and every intermediate frame. The demo app saves them as PNGs.

### 2.13 Test plan

| Test | Checks |
|---|---|
| Worked example above | exact M, exact seam [1, 1, 2, 3], energy 7 |
| **Exhaustive oracle** on random small matrices (H ≤ 6, W ≤ 6, many seeds) | DP energy = brute-force minimum. The seam is valid and its energy matches |
| Uniform energy | Every seam costs the same; tie rule gives column 0 straight down |
| High-energy barrier (a wall of large values with one gap) | Seam passes through the gap |
| Seam along the left / right border | Boundary handling (+∞ neighbours) is correct |
| 1-column / 1-row images | Degenerate shapes don't crash |
| Rectangular H ≠ W, both orientations | No H/W mix-ups. Horizontal seam has length W |
| Seam removal | Size −1, remaining pixels unchanged and in order, RGB and greyscale |
| Repeated resizing | Exact target size; invalid targets throw; the energy is recomputed every step (count the calls) |
| Determinism | Same input → identical seam every run |

The brute-force oracle lives in `tests/` only (never in the library). Teammates can reuse it to test greedy (greedy energy ≥ optimum) and graph cut (graph cut energy = optimum).

---

## 3. Graph Cut seam carving (item 4)

Part 1 (WN, `seam_graph.*`, `max_flow.*`) is implemented and tested in `tests/test_graph_cut_part1.cpp`. Part 2 (KO, `graph_cut.cpp`) reads the seam out of the result.

### 3.1 The graph (part 1)

One node per pixel, plus a source s and a sink t. A vertical seam splits every row into a left part (source side S) and a right part (sink side T). In each row the last pixel of the left part is removed.

| # | Edge | Capacity | Why |
|---|---|---|---|
| 1 | s → (i, 0) | ∞ | column 0 is always in S, so every row is cut |
| 2 | (i, j) → (i, j+1), and (i, W−1) → t | E[i][j] | cutting it removes pixel (i, j) and pays its energy |
| 3 | (i, j+1) → (i, j) | ∞ | S is a prefix of each row, so each row is cut exactly once |
| 4 | (i, j) → (i−1, j−1) and (i, j) → (i+1, j−1) | ∞ | cut positions in neighbouring rows differ by ≤ 1 (connected seam) |

- A cut's cost counts only edges going S → T. Any cut that breaks a seam rule would cut an ∞ edge, so every finite cut is a valid seam and its cost is the seam's energy.
- **Deviation from the brief:** the brief makes (i, W−1) → t infinite, so the last column could never be removed. Here it costs E[i][W−1], so the set of possible cuts matches DP's set of seams exactly.
- Energies are scaled by 1000 and rounded to integers, so the flow is exact. ∞ = 2⁵⁰.
- Size: V = HW + 2 nodes, E = H + HW + H(W−1) + 2(H−1)(W−1) ≈ 4HW edges.

### 3.2 Max-flow (part 1) — Edmonds-Karp

Ford-Fulkerson where each augmenting path is a **shortest** path, found by BFS in the residual graph. Each round: BFS from s → bottleneck capacity along the path → push it (forward edges lose capacity, reverse edges gain it). Stops when t is unreachable. By the **max-flow min-cut theorem**, the flow value then equals the capacity of the minimum cut, which is the minimum seam energy. Time O(V·E²) = O((HW)³) in the worst case; space O(V + E) = O(HW).

Measured on one random 100 × 100 energy map: 3 ms to build the graph, about 0.5 s for max-flow, so graph cut is far slower than DP's Θ(HW). Real images and the full comparison come from the benchmark harness.

### 3.3 Seam extraction (part 2)

After max-flow, S = every node reachable from s through edges with remaining capacity > 0. In row i, the seam pixel is the rightmost column in S. Horizontal seams use the transposed energy map. When several seams tie on energy, graph cut and DP may return different seams of the same cost.

---

## 4. Alternatives considered

| Alternative | Why not the default |
|---|---|
| Top-down recursion with memoisation for DP | Same Θ(HW) table, plus recursion depth H, which can overflow the call stack on tall images. Good for explaining the idea in the report. |
| Python + NumPy | Shorter code, but graph cut's max-flow loops would be far too slow in pure Python. One language for all three methods keeps the timing comparison fair. |
| OpenCV for image I/O and Sobel | Allowed by the brief, but every member would have to install and link it on Windows. stb_image needs no install (proposal, FF decides). |
| Dinic's algorithm for max-flow | Faster in practice (blocking flows), but the brief names Ford-Fulkerson / Edmonds-Karp. Mentioned in the report as an improvement. |
| Library seam carving (e.g. scikit-image's old `seam_carve`) | **Forbidden** by the brief. |

## 5. References

1. S. Avidan, A. Shamir. *Seam Carving for Content-Aware Image Resizing.* ACM SIGGRAPH 2007.
2. M. Rubinstein, A. Shamir, S. Avidan. *Improved Seam Carving for Video Retargeting.* ACM SIGGRAPH 2008 (forward energy, graph-cut formulation).
3. A. Das. *Real-world dynamic programming: seam carving.* 2019.
4. T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein. *Introduction to Algorithms*, 3rd ed., ch. 26 (Ford-Fulkerson, Edmonds-Karp, max-flow min-cut theorem).
5. CSD3130 Lecture L05 — Backtracking, Greedy, Dynamic Programming (hallmarks of DP: optimal substructure, overlapping sub-problems; time–memory trade-off).
