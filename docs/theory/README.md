# Question 2 — Seam carving theory

## 2(a)(i) Number of possible vertical seams

Let $m$ be the number of rows and $n$ the number of columns, with $m\ge 1$ and $n>1$. A vertical seam is specified by a column sequence $(c_1,\ldots,c_m)$, where $1\le c_i\le n$ and $|c_i-c_{i-1}|\le 1$ for $i=2,\ldots,m$.

Consider only seams that use columns 1 and 2. In each row we can independently choose either column, because any two choices in consecutive rows differ by at most one. Each of the $2^m$ sequences gives a valid seam, and sequences that differ in any row select different pixels. Thus the total number of seams is at least $2^m$, hence $\Omega(2^m)$ as $m$ grows for every $n>1$. When $n=2$, these are all the seams, so the bound is exact.

## 2(a)(ii) Minimum-energy seam and its subproblems

Let $e[i,j]$ be the energy of pixel $(i,j)$. The cost of seam $(c_1,\ldots,c_m)$ is $\sum_{i=1}^m e[i,c_i]$. Define $M[i,j]$ as the minimum cost of a partial seam that starts anywhere in row 1 and ends at pixel $(i,j)$. We use the cumulative-cost notation from [the Question 1 design](../algorithm_design.md#23-recurrence-r1b) with the brief’s 1-based indices.

A partial seam ending at $(i,j)$ must enter from some $(i-1,k)$ with $k$ in $\{j-1,j,j+1\}$ and $1\le k\le n$. If its prefix to $(i-1,k)$ cost more than $M[i-1,k]$, we could replace the prefix with a cheaper one ending at the same pixel. The last step would still be valid, and the total cost would be lower. Conversely, any optimal prefix to a valid predecessor can be extended by adding $(i,j)$. The recurrence follows.

$$
\begin{aligned}
M[1,j] &= e[1,j] && (1\le j\le n) \\
M[i,j] &= e[i,j]+\min_{\max(1,j-1)\le k\le\min(n,j+1)} M[i-1,k]
&& (2\le i\le m,\ 1\le j\le n).
\end{aligned}
$$

The minimum seam cost is $\min_{1\le j\le n} M[m,j]$, since a full seam can end in any column of the bottom row. To recover a seam, store a minimising predecessor when computing each cell, then follow those predecessors upward from a minimising bottom-row cell.

The subproblems overlap because each $M[i,j]$ can serve as a predecessor for up to three cells in the next row. Filling the table from top to bottom solves each of the $mn$ subproblems once with at most three candidates per cell. This takes $\Theta(mn)$ time and $\Theta(mn)$ space for the full table and predecessor pointers. Backtracking takes $\Theta(m)$ time.

The [worked example from Question 1](../algorithm_design.md#25-worked-example) gives the following cumulative costs.

$$
e=
\begin{bmatrix}
3&1&4&1&5\\
9&2&6&5&3\\
5&8&1&9&7\\
2&6&5&3&5
\end{bmatrix}
\qquad
M=
\begin{bmatrix}
3&1&4&1&5\\
10&3&7&6&4\\
8&11&4&13&11\\
10&10&9&7&16
\end{bmatrix}.
$$

For instance, $M[3,3]=1+\min(3,7,6)=4$. The smallest bottom-row cost is $M[4,4]=7$. Backtracking gives columns $(2,2,3,4)$ from top to bottom, with total energy $1+2+1+3=7$.

## 2(b) Divide and conquer for Sobel convolution

A $3\times3$ Sobel kernel uses a fixed number of neighbouring pixels per output. Computing the horizontal and vertical derivatives and combining them into an energy value therefore takes $\Theta(mn)$ time for a full energy map. Writing the $mn$ output values already requires $\Omega(mn)$ work.

One divide-and-conquer approach is to recursively split the output grid into tiles, filter each tile, and combine the outputs. A tile requires a one-pixel border of input from its neighbours, including the corners, so that outputs at tile boundaries use the same $3\times3$ neighbourhoods as in a whole-image pass. At the image boundary, use the same padding rule as for the whole image. Every output still requires constant work, so the tiled method still takes $\Theta(mn)$ total work when tiles write directly into the output. Tiling can improve cache locality or let workers process tiles in parallel. Any speedup in elapsed time depends on factors such as tile size, scheduling and memory bandwidth.

Sobel also has a separable kernel. The horizontal derivative uses the one-dimensional derivative weights $[-1,0,1]$ across columns and the smoothing weights $[1,2,1]$ down rows. For the vertical derivative, the roles of the two axes are swapped. Two one-dimensional passes per derivative reduce the number of kernel terms compared with a dense $3\times3$ evaluation while keeping time at $\Theta(mn)$. [OpenCV Sobel documentation](https://docs.opencv.org/doc/doxygen/html/d4/d86/group__imgproc__filter.html)

For a much larger dense $k\times k$ kernel, direct evaluation of an $m\times n$ output takes $\Theta(mnk^2)$ time. An FFT-based method uses divide and conquer in the transform. It pads the inputs, multiplies their Fourier transforms pointwise and applies the inverse transform. For zero-padded linear convolution, the transform dimensions must satisfy $P\ge m+k-1$ and $Q\ge n+k-1$. The resulting $O(PQ\log(PQ))$ time can be lower than direct evaluation for large kernels. For fixed $3\times3$ Sobel, the transform adds a logarithmic factor to an already linear calculation, so separable spatial filtering is the better fit. [SciPy guide to FFT convolution](https://docs.scipy.org/doc/scipy/tutorial/fft.html#discrete-fourier-transforms)
