# Item 5 — the number of seams grows at least exponentially (Q2(a)(i))

Owner: HM (Lee Hwee Min)

## Set-up

The image has m rows and n columns, with n > 1. A vertical seam picks one column in every
row: a sequence (j_1, j_2, …, j_m) with 1 ≤ j_i ≤ n and |j_{i+1} − j_i| ≤ 1
(the next pixel is directly below or diagonally below). Let S(m, n) be the number of seams.

## Claim

S(m, n) ≥ n · 2^(m−1) ≥ 2^m, so the number of seams is Ω(2^m).

## Proof (induction on the rows)

Let P_i be the number of partial seams that cover rows 1 … i.

- **Base case.** P_1 = n: the seam can start in any column.
- **Step.** Take a partial seam that ends in column j of row i. It can continue to any column
  in {j−1, j, j+1} that lies inside 1 … n.
  - An interior column (1 < j < n) has 3 choices.
  - The left border (j = 1) has {1, 2}, and the right border (j = n) has {n−1, n}.
    Both exist because n ≥ 2.

  So every partial seam has **at least 2** extensions. Different extensions differ in row
  i+1, and extensions of different partial seams already differ in rows 1 … i, so no seam
  is counted twice. Therefore P_{i+1} ≥ 2 · P_i.
- **Conclusion.** S(m, n) = P_m ≥ n · 2^(m−1) ≥ 2 · 2^(m−1) = 2^m. ∎

## Upper bound and tightness

Each pixel has at most 3 continuations, so S(m, n) ≤ n · 3^(m−1). Together:

    n · 2^(m−1)  ≤  S(m, n)  ≤  n · 3^(m−1)

- **Why n > 1 matters:** with n = 1 there is exactly one seam (straight down).
- **The lower bound is tight for n = 2:** every pixel has exactly 2 continuations, so
  S(m, 2) = 2^m.
- **Check:** a 4 × 5 image has 95 seams, and 5 · 2^3 = 40 ≤ 95 ≤ 135 = 5 · 3^3.
  `tests/test_greedy.cpp` counts seams exactly and checks the bound for m ≤ 15, 2 ≤ n ≤ 10.
- Horizontal seams are the same argument with m and n swapped.

## Why this matters

Brute force must look at ≥ 2^m seams, each costing Θ(m) to add up: Ω(m · 2^m) time. For
m = 100 that is more than 10^30 seams. This is why we need Dynamic Programming (Θ(mn), always
optimal) or Greedy (Θ(m + n), fast but not always optimal: it follows only one path).