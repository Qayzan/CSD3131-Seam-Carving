"""Plain helper functions and data shared by several test files (imported, not fixtures)."""

from __future__ import annotations

import numpy as np

from seam_carving.interfaces import Orientation, Seam

# The 4x5 energy matrix from docs/algorithm_design.md §2.5. Its cheapest vertical seam is
# [1, 1, 2, 3] with energy 7 (checked there by enumerating all 95 seams).
WORKED_ENERGY = np.array(
    [
        [3, 1, 4, 1, 5],
        [9, 2, 6, 5, 3],
        [5, 8, 1, 9, 7],
        [2, 6, 5, 3, 5],
    ],
    dtype=np.float64,
)


def random_seam(rng: np.random.Generator, shape: tuple[int, ...], orientation: Orientation) -> Seam:
    """A random *valid* seam for ``shape``: a random walk that moves -1, 0 or +1 per step."""
    h, w = shape[0], shape[1]
    length, limit = (h, w) if orientation is Orientation.VERTICAL else (w, h)
    idx = np.empty(length, dtype=np.int64)
    idx[0] = rng.integers(limit)
    for k in range(1, length):
        idx[k] = np.clip(idx[k - 1] + rng.integers(-1, 2), 0, limit - 1)
    return Seam(orientation, idx, 0.0)


# --- Independent oracles for the DP tests (they live in tests/ only, never in the package) ---


def all_vertical_seams(height: int, width: int):
    """Yield every vertical seam of an ``height`` × ``width`` grid as a tuple of columns.

    Exhaustive enumeration: start in any column, then move −1, 0 or +1 per row. There are
    up to ``width · 3^(height−1)`` of them, so only use this for tiny grids (≤ 6 × 6).
    """

    def extend(prefix):
        if len(prefix) == height:
            yield tuple(prefix)
            return
        last = prefix[-1]
        for col in (last - 1, last, last + 1):
            if 0 <= col < width:
                yield from extend(prefix + [col])

    for start in range(width):
        yield from extend([start])


def brute_force_find_seam(energy: np.ndarray, orientation: Orientation) -> Seam:
    """The truly cheapest seam, found by trying every seam (a ``SeamFinder`` for tests).

    Ties are broken like the DP: among the cheapest seams, the one whose columns read from
    the BOTTOM row upwards are smallest (lexicographically). Costs are summed top → bottom in
    the same order as the DP, so integer-valued energies give exactly comparable totals.
    """
    e = np.asarray(energy, dtype=np.float64)
    grid = e if orientation is Orientation.VERTICAL else e.T
    h, w = grid.shape
    best_key, best_cols = None, None
    for cols in all_vertical_seams(h, w):
        cost = 0.0
        for row, col in enumerate(cols):
            cost += grid[row, col]
        key = (cost, cols[::-1])
        if best_key is None or key < best_key:
            best_key, best_cols = key, cols
    return Seam(orientation, np.array(best_cols, dtype=np.int64), best_key[0])


def reference_cumulative_energy(energy: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """The DP recurrence written as plain Python loops — slow but obviously correct.

    Used to check the vectorised ``cumulative_energy`` cell by cell (design §3: "pure Python
    loops ... kept as an independent reference in the tests").
    """
    e = np.asarray(energy, dtype=np.float64)
    h, w = e.shape
    table = np.zeros((h, w))
    back = np.zeros((h, w), dtype=np.int8)
    table[0] = e[0]
    for i in range(1, h):
        for j in range(w):
            best_offset, best_value = None, None
            for offset in (-1, 0, 1):  # this order + strict "<" = leftmost wins on ties
                k = j + offset
                if 0 <= k < w and (best_value is None or table[i - 1, k] < best_value):
                    best_offset, best_value = offset, table[i - 1, k]
            table[i, j] = e[i, j] + best_value
            back[i, j] = best_offset
    return table, back
