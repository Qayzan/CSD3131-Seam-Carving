# Item 7 — Can Divide and Conquer speed up the Sobel convolution? (Q2(b))

Owner: FF (Fauzan Fauzy). Code: `src/seam_carving/energy.cpp` (`sobelEnergy`, `sobelEnergySeparable`, `sobelEnergyDivideAndConquer`). Tests: `tests/test_energy.cpp`. Notation: the image has m rows and n columns, N = mn pixels.

## Short answer

**No asymptotic improvement is possible, and on one processor there is no practical one either.** A 3×3 Sobel filter already runs in Θ(mn) time, and every algorithm needs Ω(mn), because it has to write mn output values. Divide and conquer (splitting the image into tiles) does the same Θ(mn) work plus a little overhead. Its real value is that the tiles are independent, so they can be **processed in parallel**. On p cores that can cut the elapsed time towards Θ(mn/p), until memory bandwidth becomes the limit. For a fixed 3×3 kernel, the other classic speed-ups do not help: separable kernels change only the constant factor, and FFT convolution is asymptotically worse.

## 1. Cost of the direct method

For every pixel (r, c) the energy is e = |Gx| + |Gy|. Gx and Gy are weighted sums over the 3×3 neighbourhood with fixed weights (−1, 0, +1 / 1, 2, 1). That is a constant number of operations per pixel (6 non-zero weights per kernel). So

  T_direct(m, n) = c · mn = **Θ(mn)** time, and Θ(mn) memory for the greyscale copy and the result.

Lower bound: the output has mn values, and each one depends on its own neighbourhood. Any method, sequential or divide and conquer, must therefore do Ω(mn) work. **The direct method is already asymptotically optimal**, so divide and conquer cannot change the order of growth. It can only change constants or how the work is spread across processors.

## 2. A divide-and-conquer formulation

`sobelEnergyDivideAndConquer(image, leafPixels)` does exactly this:

1. **Divide:** cut the region along its longer side into two halves.
2. **Conquer:** recurse on each half. A region with at most L = `leafPixels` pixels is filtered directly.
3. **Combine:** nothing. Each output pixel depends only on its own 3×3 neighbourhood, so each half writes its own part of the result and no merging is needed.

**Boundary halos.** A pixel on the edge of a tile needs neighbours that belong to the next tile. So each tile *reads* a 1-pixel halo around itself (it never writes it). At the outer image border the same reflect-101 rule as the whole-image pass applies. With these two rules every pixel sees exactly the same 3×3 neighbourhood as in the direct pass. The results are **bit-for-bit identical** (tested for leaf sizes 1, 2, 7, 64 and 2²⁰ on 9 image shapes).

**Recurrence.** For N pixels, T(N) = 2T(N/2) + Θ(1) for N > L, and T(N) = Θ(N) for N ≤ L. By the Master theorem (a = 2, b = 2, f(N) = Θ(1) = O(N^(1−ε))) this is case 1: T(N) = Θ(N) = **Θ(mn)**. The recursion adds about 2N/L calls, a lower-order term.

**Halo overhead.** A leaf of h × w pixels reads (h+2)(w+2) inputs for hw outputs, about 2/h + 2/w extra. For square leaves of side s = √L that is about 4/s: 6% for 64 × 64 tiles and 1.6% for 256 × 256 tiles, but 9 reads per output for 1-pixel tiles. In our implementation the halos are read in place, not copied. If tiles are copied (as on a GPU or across machines), the same small fraction of extra memory and transfer is needed.

## 3. Measurements (our code, one core)

Command: `seam_carver --bench-sobel IMAGE --repeat 15`. The demo app runs each version, reports the best of 15 runs, and checks every version against `sobelEnergy`. Release build (g++ 11.4, `-O3`), Linux VM on an AMD Ryzen 9 8945HS. Times include the greyscale conversion, which is the same for every version. The photo is the classic seam-carving test picture `Broadway_tower_edit.jpg` (Wikimedia Commons, CC BY-SA). It is not in the repository; any photo of similar size gives similar timings. The synthetic image is `assets/images/synthetic_scene.png`.

| Version | 968 × 1428 photo | 300 × 480 synthetic | max \|difference\| |
|---|---|---|---|
| direct 3×3 (`sobelEnergy`) | 4.3 – 4.5 ms | 0.40 ms | — |
| separable 1×3 + 3×1 | 5.8 – 6.1 ms | 0.41 ms | 2.3 × 10⁻¹³ (rounding) |
| divide & conquer, L = 64 | 7.6 – 7.8 ms | 0.46 ms | 0 |
| divide & conquer, L = 1 024 | 5.7 ms | 0.44 ms | 0 |
| divide & conquer, L = 16 384 | 4.7 ms | 0.43 ms | 0 |
| divide & conquer, L = 262 144 | 5.1 ms | 0.42 ms | 0 |

What the table shows:

- With small tiles, recursion overhead (function calls, bounds bookkeeping) costs up to about 75% extra.
- With tiles of 16 384 pixels or more, divide and conquer is within about 5–15% of the direct pass, never meaningfully faster.

The direct pass streams through the picture row by row. It needs only 3 rows of the greyscale image at a time (3 × 1 428 × 8 bytes ≈ 34 KB, about one L1 cache). So its **memory locality is already good**, and tiling has nothing left to improve for a 3×3 kernel. Tiling improves locality when the kernel is large, or when several passes are fused per tile (for example greyscale conversion + Sobel + the first DP row) so the data stays in cache between passes.

## 4. Sequential versus parallel

The real benefit of divide and conquer is **independence**: the two halves share no writes, so they can run at the same time.

- **Work** (total operations) W = Θ(mn). **Span** (longest chain of dependent steps) with recursive forking = Θ(log(mn/L)) + Θ(L).
- By Brent's bound, on p processors T_p ≤ W/p + span = **Θ(mn/p + log(mn) + L)**. For large images that is close to a p-fold speed-up.
- **Limits:**
  1. **Memory bandwidth.** Sobel does about a dozen arithmetic operations per pixel and must stream every byte of the image, so it is memory-bound. Once all cores together saturate memory bandwidth, adding cores stops helping.
  2. **Overhead.** Starting threads or tasks costs microseconds. For the 300 × 480 test image the whole pass takes 0.4 ms, so per-seam threading would gain little.
  3. **Amdahl's law in seam carving.** The DP is sequential from row to row, and seam t+1 depends on seam t. Even a perfectly parallel energy step speeds up only that part of each iteration (item 10).
- We did not add threads to the library. The CMake setup does not link a thread library, and a fair parallel benchmark needs more than the 2-core VM used here. The divide-and-conquer function is written so each recursive call could be handed to `std::async` or an OpenMP task without other changes.

## 5. Separable kernels

Each Sobel kernel is an outer product of two 1-D kernels:

```
Gx kernel = [1 2 1]ᵀ · [−1 0 1]        Gy kernel = [−1 0 1]ᵀ · [1 2 1]
```

So each derivative can be computed as a 1×3 pass along the rows followed by a 3×1 pass down the columns (`sobelEnergySeparable`). For a general k × k separable kernel this cuts the cost per pixel from k² to 2k multiply-adds: **Θ(mnk²) → Θ(mnk)**, a real gain for large k. For k = 3 the saving (9 → 6 terms) is smaller than the cost of the extra pass and the two temporary maps, which is why it measured *slower* (about 1.3× on the photo). Either way it is still Θ(mn). Separability changes the constant factor, not the order of growth.

## 6. FFT convolution: divide and conquer in the frequency domain

The FFT is itself a divide-and-conquer algorithm. Convolution by FFT costs O(mn log(mn)) for any kernel size (after zero-padding), against Θ(mnk²) directly. It wins when k² is larger than about log(mn), so for big blur kernels. For k = 3 the direct method's Θ(mn) is **asymptotically better** than O(mn log mn), so FFT would make the energy map slower.

## 7. What actually speeds up the energy step in seam carving

The brief requires the energy map to be recalculated after every seam (Task 1(c)), and our carver recomputes it in full: Θ(mn) per seam. After removing a vertical seam, the only pixels whose 3×3 neighbourhood changed are those within a couple of columns of the removed pixel in each row (seam positions in neighbouring rows differ by at most 1). Updating just those gives exactly the same map in Θ(m) instead of Θ(mn). This is a better optimisation than divide and conquer, and an input for item 10 (improvements). The seam removal itself still costs Θ(mn) per seam, because the image has to be shifted.

## 8. Answer for the report (Q2(b))

> Computing the Sobel gradient with a 3×3 kernel takes Θ(mn) time for an m × n image, a constant amount of work per pixel. This is optimal, because any method must produce mn output values. A divide-and-conquer version splits the image recursively into tiles, filters each tile with a 1-pixel boundary halo, and needs no combine step. Its recurrence T(N) = 2T(N/2) + Θ(1) solves to Θ(N) = Θ(mn) by the Master theorem (case 1), so it does not improve the asymptotic running time. Our measurements confirm this: on one core the tiled version was never meaningfully faster than the direct pass, and with small tiles it was up to 75% slower because of recursion overhead. The benefit of divide and conquer is that the tiles are independent. On p processors the elapsed time can drop towards Θ(mn/p + log mn), limited in practice by memory bandwidth and thread start-up costs, since the filter does little arithmetic per byte. Separable kernels reduce the per-pixel cost from k² to 2k operations, a constant-factor saving that only matters for large kernels (for 3×3 it measured slightly slower). FFT convolution, also divide and conquer, costs O(mn log mn) and is only worthwhile for large kernels. For seam carving, a larger gain comes from updating the energy only next to the removed seam (Θ(m) instead of Θ(mn) per seam).

## 9. Presentation version (about 30 seconds)

> "Q2(b) asks if divide and conquer makes the Sobel filter faster. Sobel looks at 9 pixels around each pixel, so it is already linear in the number of pixels, and nothing can beat linear, because you have to write every output. If we split the image into tiles, the recurrence 2T(N/2) + constant is still Θ(N). We measured it: on one core, tiling is the same speed or slower. Where it helps is parallelism: tiles don't depend on each other, so cores can work on them at the same time, until memory bandwidth runs out. Separable kernels and FFT only pay off for big kernels, not for a 3 by 3 one."

## References

- I. Sobel, G. Feldman, "A 3×3 isotropic gradient operator for image processing" (1968), as described in R. C. Gonzalez, R. E. Woods, *Digital Image Processing*, 4th ed., §3.6 and §10.2.
- T. H. Cormen et al., *Introduction to Algorithms*, 3rd ed.: ch. 4 (Master theorem), ch. 27 (multithreaded algorithms: work, span, Brent's bound), ch. 30 (FFT).
- S. Avidan, A. Shamir, "Seam Carving for Content-Aware Image Resizing", SIGGRAPH 2007 (the e1 energy).
