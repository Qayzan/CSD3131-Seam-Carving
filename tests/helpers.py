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
