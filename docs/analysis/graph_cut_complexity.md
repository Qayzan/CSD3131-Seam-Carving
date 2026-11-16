
# Graph Cut — Implementation and Analysis

## 1. Algorithm Overview

Our Graph Cut implementation uses a flow network to find the minimum-energy seam in an image. The graph is constructed from the image's energy map, and the Edmonds–Karp algorithm is used to calculate the maximum flow.

After max-flow is completed, Breadth-First Search (BFS) is performed on the residual graph to identify the source side of the minimum cut.

For each row, the rightmost reachable pixel is selected as the seam position. The extracted seam is then checked to ensure that consecutive seam positions differ by at most one column.

The implementation consists of two parts:

- **Graph construction and max-flow:** Implemented using `seam_graph.cpp` and `max_flow.cpp`.
- **Seam extraction:** Implemented using `seam_extraction.cpp`.

## 2. Time and Space Complexity (Question 5b)

Let:

- $m$ = number of rows (image height)
- $n$ = number of columns (image width)
- $V$ = number of vertices in the graph
- $E$ = number of edges in the graph

### 2.1 Graph Construction

Each pixel is represented by one node, with two additional nodes for the source and sink.

The total number of vertices is:

$$
V = mn + 2 = O(mn)
$$

Each pixel is connected to only a constant number of neighbouring nodes. Therefore, the number of edges is also $O(mn)$.

Since every pixel is processed when constructing the graph, both the time and space complexity of graph construction are $O(mn)$.

### 2.2 Maximum Flow (Edmonds–Karp)

Our implementation uses the Edmonds–Karp algorithm, which repeatedly performs BFS to find augmenting paths in the residual graph.

Its worst-case time complexity is:

$$
O(VE^2)
$$

Since both $V$ and $E$ are $O(mn)$, substituting gives:

$$
O(mn \times (mn)^2) = O(m^3n^3)
$$

This is the most computationally expensive part of our Graph Cut implementation.

The graph is stored using adjacency lists and residual edges, requiring $O(V+E)$ space, which simplifies to $O(mn)$.

### 2.3 Seam Extraction

After maximum flow is calculated, we perform BFS from the source through edges with positive residual capacity.

This identifies all nodes on the source side of the minimum cut.

For each row, we select the rightmost reachable pixel as the seam position. We also verify that consecutive seam positions differ by at most one column.

BFS takes $O(V+E)$ time, while scanning the pixels takes $O(mn)$ time.

Therefore, seam extraction takes $O(mn)$ time overall and $O(mn)$ auxiliary space for the reachable-node array and BFS queue. The returned seam itself requires $O(m)$ space.

### 2.4 Overall Complexity

| Operation | Time Complexity | Space Complexity |
|---|---|---|
| Graph construction | $O(mn)$ | $O(mn)$ |
| Edmonds–Karp | $O(m^3n^3)$ | $O(mn)$ |
| Residual BFS and seam extraction | $O(mn)$ | $O(mn)$ |
| **Overall per seam** | **$O(m^3n^3)$** | **$O(mn)$** |

The total worst-case time complexity is dominated by Edmonds–Karp.

Although Graph Cut can find an optimal seam for the chosen energy function, its worst-case time complexity is higher than the $O(mn)$ Dynamic Programming approach. This makes Graph Cut potentially more expensive when processing larger images or repeatedly removing seams.

These are theoretical worst-case bounds. Preliminary runtime measurements using synthetic energy maps are presented below. A final comparison with Dynamic Programming will be performed after integration.

## 3. Correctness Testing

We tested the Graph Cut seam extraction implementation using six small energy maps, covering straight seams, diagonal seams, and single-row or single-column images.

The tests were written in `tests/test_seam_extraction.cpp`.

For each test, we checked that:

- The extracted seam contains one pixel per row.
- All seam positions are within the image boundaries.
- Consecutive seam positions differ by at most one column.
- The extracted seam energy matches the minimum-cut cost.
- The seam energy matches a reference Dynamic Programming implementation.

### 3.1 Test Results

| Test Case | Extracted Seam | Result |
|---|---|---|
| 3×3 leftmost seam | [0, 0, 0] | PASS |
| 3×3 diagonal seam | [2, 1, 0] | PASS |
| 1×1 image | [0] | PASS |
| 1×4 image | [1] | PASS |
| 3×1 image | [0, 0, 0] | PASS |
| 3×3 middle seam | [1, 1, 1] | PASS |

All six tests passed.

These tests confirm that Graph Cut correctly extracts minimum-energy vertical seams for the tested energy maps.

The reference DP implementation is used only for standalone correctness testing. Comparison against the team's actual DP implementation will be carried out during integration.

## 4. Runtime Benchmarking

The Graph Cut implementation was benchmarked using synthetic square energy maps with a fixed random seed (42).

The benchmark measures three stages:

1. Graph construction
2. Edmonds–Karp maximum flow
3. Seam extraction

The program was compiled using g++ 13.3.0 with C++17 and `-O2` optimisation under WSL Ubuntu.

### 4.1 Preliminary Benchmark Results (Single Run)

| Image Size | Graph Construction | Max-Flow | Seam Extraction | Total |
|---|---|---|---|---|
| 10×10 | 0.042 ms | 0.069 ms | 0.004 ms | 0.114 ms |
| 20×20 | 0.127 ms | 1.266 ms | 0.012 ms | 1.405 ms |
| 30×30 | 0.245 ms | 4.282 ms | 0.015 ms | 4.543 ms |
| 40×40 | 0.646 ms | 16.063 ms | 0.046 ms | 16.755 ms |

The initial results showed that the total execution time increased as the image size grew. Maximum flow took considerably longer than graph construction and seam extraction.

However, a single benchmark run may be affected by background processes and timing fluctuations. Therefore, additional runs were performed to obtain more reliable measurements.

### 4.2 Repeated Benchmark Results (5 Runs)

To reduce the effect of timing fluctuations, the Graph Cut benchmark was repeated five times using the same synthetic energy maps.

The median runtime was calculated separately for each stage.

| Image Size | Graph Construction | Max-Flow | Seam Extraction | Total |
|---|---|---|---|---|
| 10×10 | 0.042 ms | 0.065 ms | 0.003 ms | 0.111 ms |
| 20×20 | 0.178 ms | 1.330 ms | 0.013 ms | 1.611 ms |
| 30×30 | 0.328 ms | 4.713 ms | 0.023 ms | 5.057 ms |
| 40×40 | 0.667 ms | 18.604 ms | 0.048 ms | 19.344 ms |

### 4.3 Observations and Discussion

The results show that execution time increases as the image size grows.

Graph construction and seam extraction remain relatively fast compared to maximum flow.

For the 40×40 energy map, the median maximum-flow time was 18.604 ms, compared to the median total time of 19.344 ms.

This shows that Edmonds–Karp is the main performance bottleneck in our implementation, which is consistent with its higher theoretical worst-case time complexity.

We also observed some variation in runtime between repeated runs. This may be due to system load, memory allocation, or other background processes.

However, these measurements only represent performance on small synthetic energy maps. Larger images and different energy distributions may produce different results.

The median values for each stage were calculated independently, so their sum may not exactly equal the median total runtime.


## 5. Comparison with Dynamic Programming (Question 5c)

We compared Graph Cut against the team's actual Dynamic Programming
implementation using identical synthetic energy maps.

Eight correctness tests were performed, consisting of five vertical
and three horizontal seam cases. Horizontal seams were tested by
transposing the energy map before running Graph Cut.

Both algorithms produced the same minimum seam cost in all eight tests.

### 5.1 Runtime Comparison

Both algorithms were benchmarked using synthetic square energy maps.
Each test was repeated five times, and the median runtime was recorded.

| Image Size | DP (Median) | Graph Cut (Median) |
|---|---:|---:|
| 10×10 | < 0.001 ms | 0.052 ms |
| 20×20 | 0.002 ms | 0.734 ms |
| 30×30 | 0.006 ms | 2.640 ms |
| 40×40 | 0.011 ms | 10.763 ms |

Dynamic Programming was consistently faster than Graph Cut.

At 40×40, DP took approximately 0.011 ms, while Graph Cut took
10.763 ms.

This is expected because DP processes each pixel once, giving
O(mn) time complexity, while Graph Cut performs graph construction
and maximum-flow calculations.

The results are consistent with Edmonds-Karp's worst-case
time complexity of O((mn)^3).

These benchmarks measure vertical seam finding using synthetic
energy maps rather than complete image resizing.



## 6. Final Integration and Evaluation

We tested Graph Cut and Dynamic Programming using the team's
shared seam-carving implementation.

The original synthetic scene was scaled to 80×50 pixels for testing.
Both algorithms then resized the image to 75×47 pixels by removing
five vertical seams and three horizontal seams.

The same Sobel energy calculation, seam-removal logic and resizing
order were used for both algorithms.

### 6.1 Real-Image Test Results

| Measurement | Dynamic Programming | Graph Cut |
|---|---:|---:|
| Input dimensions | 80×50 | 80×50 |
| Output dimensions | 75×47 | 75×47 |
| Seam-finding time | 0.095 ms | 41.666 ms |
| Total resizing time | 0.275 ms | 41.851 ms |

Both algorithms successfully produced images with the requested
dimensions.

The output images were also compared pixel by pixel. There were
zero differing pixel-channel values, meaning both algorithms
produced identical resized images for this test.

### 6.2 Discussion

Although both algorithms produced the same visual result, Graph Cut
was considerably slower.

DP completed the resizing in 0.275 ms, while Graph Cut took
41.851 ms overall.

Most of Graph Cut's execution time was spent finding seams,
rather than calculating energy maps or removing pixels.

The results suggest that Dynamic Programming is more efficient
for this seam-carving task, while Graph Cut provides an alternative
minimum-cut formulation.

These results are based on one downscaled synthetic scene and
one resizing configuration. Additional images and repeated
measurements would provide a broader evaluation.

