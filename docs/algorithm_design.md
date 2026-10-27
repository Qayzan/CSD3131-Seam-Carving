# Architecture and Question 1 design

Status: **design proposal (Milestone 1)**. Implementation starts in Milestone 2 (shared utilities) and Milestone 3 (Question 1). Sections marked *decision* are proposals the team can still change before code depends on them.

---

## 1. Architecture

### 1.1 Stack and why

| Choice | Reason |
|---|---|
| **Python 3.10+** | Readable reference code for the report and the quiz. Every member can run it. Fast enough once the inner loops use NumPy. |
| **NumPy** | Images and energy maps are 2-D/3-D arrays. Row-wise vectorisation makes the DP fast without unreadable code. |
| **OpenCV (`opencv-python-headless`)** | Image I/O and `cv2.Sobel`, which the brief explicitly allows. The "headless" build installs cleanly in CI. |
| **pytest** | Standard, simple tests; parametrised cases for many image sizes. |
| **ruff** | One fast tool for linting and formatting, so style debates don't take up review time. |
| **GitHub Actions** | Runs lint + tests on every pull request. This is the CI part of bonus item 7(c). |

No frontend framework yet. A small React (or Streamlit) UI could be added later for bonus 7(a)/(b). The `carver` module will already expose every intermediate step such a UI needs, so the UI stays a thin layer.

### 1.2 Module map and data flow

```
              image_utils.load_image()                 (RGB uint8, H×W×3)
                         │
                         ▼
     ┌────────── carver.carve(image, target, finder) ──────────┐
     │  loop until target size:                                │
     │    E    = energy.gradient_energy(image)    (H×W float64)│
     │    seam = finder(E, orientation)           (Seam)       │  ◄── dynamic_programming (FF)
     │    seam_ops.validate_seam(seam, E.shape)                │  ◄── greedy             (HM)
     │    image = seam_ops.remove_seam(image, seam)            │  ◄── graph_cut          (WN)
     │    (optionally record frame / seam for visualisation)   │
     └─────────────────────────────────────────────────────────┘
                         │
                         ▼
              CarveResult(image, seams, timings, frames)  →  examples/, benchmarks/, future UI
```

The three algorithms differ **only** in `finder`. Everything else is shared, so their comparison (Q3, Q4, Q5) is fair: the same energy function, the same removal code, the same timing method.

### 1.3 Conventions (summary — full contract in [interfaces.md](interfaces.md))

- `image[row, col]`, row 0 at the top. RGB order inside the package. BGR only at the OpenCV boundary.
- Energy: `float64`, shape `(H, W)`, non-negative.
- Vertical seam: `cols`, int array of length `H`. `cols[i]` is the column removed from row `i`, and `|cols[i] − cols[i−1]| ≤ 1`.
- Horizontal seam: `rows`, int array of length `W`. `rows[j]` is the row removed from column `j`, and `|rows[j] − rows[j−1]| ≤ 1`.
- The brief uses 1-based `img[1…m, 1…n]`. The code uses 0-based indices. The report will say so once.

---

## 2. Question 1 — Dynamic Programming seam carving

### 2.1 Energy map (R1a) — *decision*

Use the **e1 gradient-magnitude energy** of Avidan & Shamir (2007):

$$e(I) = \left|\frac{\partial I}{\partial x}\right| + \left|\frac{\partial I}{\partial y}\right|$$

- Convert RGB to greyscale **as float64** first. This avoids `uint8` overflow and wrap-around: `200 − 250` must be −50, not 206.
- Derivatives come from the 3×3 **Sobel** operator (`cv2.Sobel`, `ksize=3`, output `CV_64F`). Sobel smooths across the perpendicular direction, so single-pixel noise counts less than with a plain neighbour difference.
- **Borders:** OpenCV's default `BORDER_REFLECT_101` mirrors the image at the edge. Without it, border pixels would see a jump to zero and look like strong edges, so no seam would ever use the image border.
- Why this energy: it is cheap (O(HW)), it is the energy used in the original paper and in the brief, and it is high exactly where removing a pixel would be visible (edges, texture).
- Alternatives (documented for the report, not default): per-channel gradients summed over R, G, B (catches colour edges of equal brightness), and *forward energy* (Rubinstein et al., 2008). Forward energy measures the new edges a removal creates. The graph-cut question mentions it too.

### 2.2 Why dynamic programming applies

The problem: among all vertical seams (one pixel per row, neighbours in adjacent rows at most one column apart), find the one with the smallest total energy.

- **Brute force is hopeless.** Each step down has up to 3 choices, so there are up to n·3^(m−1) seams. Q2(a)(i) asks for the exponential lower bound; that proof belongs to YN. The test oracle below enumerates them only for tiny matrices.
- **Optimal substructure.** Take the cheapest seam that ends at pixel (i, j). Its part above row i is itself the cheapest seam that ends at whichever pixel (i−1, k) it passes through, with k ∈ {j−1, j, j+1}. If there were a cheaper seam ending at (i−1, k), we could swap it in and get a cheaper seam to (i, j), which is a contradiction. So optimal answers are built from optimal answers to smaller sub-problems.
- **Overlapping sub-problems.** The best seam to (i−1, k) is reused by up to three pixels below it: (i, k−1), (i, k), (i, k+1). Plain recursion would recompute it exponentially often. DP computes each of the m·n sub-problems **once** and stores it in a table: the course's time–memory trade-off.

### 2.3 Recurrence (R1b)

For energy matrix `E` (H rows, W columns), `M[i, j]` = minimum total energy of any seam from row 0 down to pixel (i, j):

```
Base case:   M[0, j] = E[0, j]                                     for 0 ≤ j < W
Recurrence:  M[i, j] = E[i, j] + min( M[i-1, j-1], M[i-1, j], M[i-1, j+1] )   for 1 ≤ i < H
             (only valid columns: out-of-range neighbours count as +∞)
Answer:      min_j M[H-1, j]
```

Alongside `M` we keep a **back-pointer table** `back[i, j] ∈ {−1, 0, +1}`, which says which predecessor gave the minimum. This makes backtracking trivial and lets us check the result.

**Tie-breaking (*decision*): deterministic, "leftmost wins".** When predecessors tie, choose the smallest column (j−1 before j before j+1). In the bottom row, choose the smallest j among equal minima. `np.argmin` already returns the first minimum, so the vectorised code and a plain loop give identical seams. The tests can therefore assert exact seams, not just exact energies.

### 2.4 Algorithm (pseudocode)

```
FIND-VERTICAL-SEAM(E):                      ▷ E is H × W
 1  M ← copy of E (float64);  back ← H × W array of int8 zeros
 2  for i ← 1 to H − 1:
 3      left  ← [+∞, M[i−1, 0 … W−2]]       ▷ M[i−1, j−1], shifted right, +∞ at the border
 4      up    ←      M[i−1, 0 … W−1]
 5      right ← [M[i−1, 1 … W−1], +∞]       ▷ M[i−1, j+1]
 6      choice ← argmin over (left, up, right) for every column   ▷ 0, 1, 2; first min wins
 7      M[i]    ← E[i] + min(left, up, right)
 8      back[i] ← choice − 1                 ▷ −1, 0, +1
 9  seam[H−1] ← argmin_j M[H−1, j]           ▷ leftmost minimum
10  for i ← H − 1 down to 1:
11      seam[i−1] ← seam[i] + back[i, seam[i]]
12  return seam, M[H−1, seam[H−1]]
```

Lines 3–8 handle a whole row at once with NumPy. The loop over rows remains, because row i needs row i−1 (the DP dependency), but the work inside each row is vectorised.

**Self-check built into the tests.** The returned energy `M[H−1, seam[H−1]]` must equal `Σ_i E[i, seam[i]]`, and the seam must be 8-connected.

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

Row 1, column 2 shows the tie rule. Its predecessors are M[0,1] = 1, M[0,2] = 4 and M[0,3] = 1. Columns 1 and 3 tie, so the leftmost (column 1) wins: back[1,2] = −1 and M[1,2] = 6 + 1 = 7.

Backtracking starts at the smallest value in the bottom row: M[3,3] = 7. Following the back-pointers gives:

```
seam (column per row) = [1, 1, 2, 3]      energies 1 + 2 + 1 + 3 = 7
```

A brute-force check of all 95 valid seams of this matrix also gives a minimum of 7 at the same seam. This example becomes a unit test.

### 2.6 Horizontal seams — *decision: transpose*

A horizontal seam in `E` is exactly a vertical seam in `Eᵀ`:

```
FIND-HORIZONTAL-SEAM(E):  rows, cost ← FIND-VERTICAL-SEAM(Eᵀ);  return rows   ▷ len(rows) = W
```

- One DP implementation serves both orientations, so there is less code to test and nothing can diverge between them.
- `E.T` is a NumPy *view* (no copy). We take one contiguous copy before the DP so the row-by-row access stays cache-friendly.
- Coordinates come back as `rows[j]` for column `j` (the contract in §1.3). Tests use **non-square** images (e.g. 4 × 7 and 7 × 4), so mixing up H and W fails loudly.

### 2.7 Seam removal (R1c)

- **Vertical:** build a boolean mask of shape `(H, W)` that is `False` exactly at `(i, cols[i])`, index the image with it, and reshape to `(H, W−1[, C])`. One vectorised pass. The dtype and all other pixel values and their order are preserved.
- **Horizontal:** remove from the transposed image and transpose back. The same code is reused.
- **Tests:** new shape is exactly one column (or row) smaller; every remaining pixel equals the original pixel at its shifted position; the seam pixels are gone; `uint8` stays `uint8`; greyscale and RGB both work.

### 2.8 Repeated resizing — *decision*

```
CARVE(image, target_h, target_w, finder):
  validate: 1 ≤ target_h ≤ H, 1 ≤ target_w ≤ W   (shrinking only; error otherwise)
  while width  > target_w:  E ← energy(image); s ← finder(E, VERTICAL);   image ← remove(image, s)
  while height > target_h:  E ← energy(image); s ← finder(E, HORIZONTAL); image ← remove(image, s)
```

- **The energy is recalculated after every removal**, as R1c requires. Removing a seam brings formerly separated pixels together, so their gradients change.
- **Order:** width first, then height. It is simple and reproducible. An `order="alternate"` option will interleave the two directions. The order that is *optimal* overall needs a second DP over (rows removed, columns removed): the "transport map" of Avidan & Shamir §3.3. We mention it in the report as an improvement and do not implement it by default (it costs much more time).
- **Enlargement** (seam insertion) is out of scope unless the team asks for it. The brief's Q1 steps only cover removal.

### 2.9 Complexity (for the report; KO may reuse)

Let the image be H × W, and remove k seams.

| Step | Time | Extra space | Notes |
|---|---|---|---|
| Energy (Sobel, 3×3) | Θ(HW) | Θ(HW) | constant work per pixel |
| DP table M | Θ(HW) | Θ(HW) floats | 3 comparisons per cell |
| Back-pointers | Θ(HW) (inside the DP) | Θ(HW) int8 | 1 byte per cell |
| Backtracking | Θ(H) | Θ(H) | one step per row |
| Seam removal | Θ(HW) | Θ(HW) (new array) | copying dominates |
| **One seam total** | **Θ(HW)** | **Θ(HW)** | |
| k vertical seams | Θ(k·H·W) | Θ(HW) | width shrinks each time, so precisely Σ H·(W−t) |
| Horizontal seam | Θ(HW) | Θ(HW) | transpose is O(1) as a view; one copy Θ(HW) |

### 2.10 Do we need the whole DP table? (Q5(d) input)

- Computing **only the minimum cost** needs just the previous row of M: **Θ(W) space** (two rolling rows).
- Recovering the **seam itself** needs to know, for every row, which predecessor was chosen. Options:
  1. Keep the full float64 M and recompute the choices while backtracking: 8·H·W bytes.
  2. **Keep two rolling rows of M plus an int8 back-pointer table**: 2·8·W + H·W bytes, about 8× less memory. *Planned implementation.*
  3. Hirschberg-style divide & conquer: Θ(W) space, but about 2× the time and a lot more complexity. Mentioned in the report only.
- Even option 2 is still Θ(HW) in total, because the seam depends on choices in every row.

### 2.11 Would multithreading help? (Q5(d) input)

- **Across rows: no.** Row i depends on row i−1, so the DP is inherently sequential in i.
- **Within a row: yes in principle.** The W cells of a row are independent, and NumPy already processes them with vectorised (SIMD) C loops. For typical image widths (hundreds to a few thousand pixels), starting threads per row costs more than it saves.
- **Energy computation:** embarrassingly parallel. OpenCV may already use several threads internally.
- **Across seams: no.** Seam t+1 depends on the image after seam t is removed.
- Timings in `benchmarks/` (Milestone 6) will back these arguments with measurements, not assumptions.

### 2.12 Visualisation hooks (for the demo and bonus 7a)

`dynamic_programming` and `carver` will optionally return: the energy map, the cumulative table M (normalised for display), the back-pointer path, seam coordinates, the image with the seam highlighted, the image after removal, and every intermediate frame. The demo script in Milestone 5 saves them as PNGs.

### 2.13 Test plan (Milestone 4)

| Test | Checks |
|---|---|
| Worked example above | exact M, exact seam [1, 1, 2, 3], energy 7 |
| **Exhaustive oracle** on random small matrices (H ≤ 6, W ≤ 6, many seeds) | DP energy = brute-force minimum. The seam is valid and its energy matches |
| Uniform energy | Every seam costs the same; tie rule gives column 0 straight down |
| High-energy barrier (a wall of large values with one gap) | Seam passes through the gap |
| Seam along the left / right border | Boundary handling (+∞ padding) is correct |
| 1-column / 1-row images | Degenerate shapes don't crash |
| Rectangular H ≠ W, both orientations | No H/W mix-ups. Horizontal seam has length W |
| Seam removal | Shape −1, remaining pixels unchanged and in order, dtype kept, RGB and greyscale |
| Repeated resizing | Exact target size; invalid targets raise errors; the energy is recomputed every step (spy/mock) |
| Determinism | Same input → identical seam every run |

The brute-force oracle lives in `tests/` only (never in the package). Teammates can reuse it to test greedy (greedy energy ≥ optimum) and graph cut (graph cut energy = optimum).

---

## 3. Alternatives considered

| Alternative | Why not the default |
|---|---|
| Top-down recursion with memoisation | Same Θ(HW) table, plus recursion depth H. That can exceed Python's recursion limit on tall images, and function calls are slow. Good for explaining the idea in the report. |
| Pure Python loops for the DP | Clearest, but much slower on real images (Python-level work for every one of the H·W cells; the actual factor will be measured in `benchmarks/`). Kept as an *independent reference* in the tests. |
| Writing the DP in C++/Numba | Faster, but harder for the whole team to read and run. NumPy row vectorisation is fast enough. |
| Library seam carving (e.g. scikit-image's old `seam_carve`) | **Forbidden** by the brief. |

## 4. References

1. S. Avidan, A. Shamir. *Seam Carving for Content-Aware Image Resizing.* ACM SIGGRAPH 2007.
2. M. Rubinstein, A. Shamir, S. Avidan. *Improved Seam Carving for Video Retargeting.* ACM SIGGRAPH 2008 (forward energy, graph-cut formulation).
3. A. Das. *Real-world dynamic programming: seam carving.* 2019.
4. CSD3130 Lecture L05 — Backtracking, Greedy, Dynamic Programming (hallmarks of DP: optimal substructure, overlapping sub-problems; time–memory trade-off).
