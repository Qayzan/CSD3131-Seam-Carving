"""Question 1 — minimum-energy seams by Dynamic Programming. Owner: FF (Fauzan Fauzy).

The idea in plain words
-----------------------
A vertical seam goes from the top row to the bottom row, one pixel per row, and may move at
most one column left or right per step. We want the seam with the smallest total energy.
Trying every seam is hopeless (up to W·3^(H−1) of them), but the problem has two DP hallmarks
(Lecture 5):

* **Optimal substructure** — the cheapest seam that ends at pixel (i, j) must arrive from the
  cheapest seam ending at one of the three pixels above it: (i−1, j−1), (i−1, j), (i−1, j+1).
* **Overlapping sub-problems** — each of those "cheapest seam to (i−1, k)" answers is reused by
  up to three pixels below it, so we compute each one once and store it in a table.

Recurrence (``E`` = energy map with H rows and W columns, 0-based)::

    M[0, j] = E[0, j]
    M[i, j] = E[i, j] + min(M[i−1, j−1], M[i−1, j], M[i−1, j+1])     (missing neighbours = +∞)
    answer  = min over j of M[H−1, j]

Next to ``M`` we store a back-pointer ``back[i, j]`` ∈ {−1, 0, +1} that says which pixel above
gave the minimum; following those pointers from the bottom row upwards gives the seam.

**Tie rule (deterministic): leftmost wins.** ``np.argmin`` returns the first minimum, so among
equal predecessors the order is j−1, then j, then j+1, and in the bottom row the smallest j.
The resulting seam is the lexicographically smallest optimal seam when read bottom → top.

**Horizontal seams** are vertical seams of the transposed energy map, so one implementation
serves both orientations (design: docs/algorithm_design.md §2.6).

Cost for an H × W map: Θ(H·W) time. ``find_seam`` keeps only two rows of ``M`` plus the
``int8`` back-pointer table (≈ 1 byte per pixel instead of 8; design §2.10).
``cumulative_energy`` returns the full table for teaching, figures and tests.
"""

from __future__ import annotations

import numpy as np

from seam_carving.interfaces import Orientation, Seam

__all__ = [
    "backtrack",
    "cumulative_energy",
    "find_horizontal_seam",
    "find_seam",
    "find_vertical_seam",
]


def _as_energy(energy: np.ndarray) -> np.ndarray:
    """Check the energy map and return it as float64 (a view when it already is float64)."""
    if not isinstance(energy, np.ndarray):
        raise TypeError(f"energy must be a numpy.ndarray, got {type(energy).__name__}")
    if energy.ndim != 2:
        raise ValueError(f"energy must be a 2-D (H, W) array, got shape {energy.shape}")
    if energy.shape[0] < 1 or energy.shape[1] < 1:
        raise ValueError(f"energy must be at least 1x1, got shape {energy.shape}")
    if energy.dtype == np.bool_ or not np.issubdtype(energy.dtype, np.number):
        raise TypeError(f"energy must be numeric, got dtype {energy.dtype}")
    if np.issubdtype(energy.dtype, np.complexfloating):
        raise TypeError("energy must be real-valued")
    values = energy.astype(np.float64, copy=False)
    if not np.all(np.isfinite(values)):
        raise ValueError("energy contains NaN or infinite values")
    return values


def _best_predecessors(prev: np.ndarray, candidates: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """For every column j, the cheapest of prev[j−1], prev[j], prev[j+1] and which one it was.

    ``candidates`` is a reusable (3, W) work buffer. Row 0 holds the "up-left" values
    (``prev`` shifted right, +∞ in column 0), row 1 "up", row 2 "up-right" (+∞ in the last
    column). ``argmin`` over the three rows gives 0, 1 or 2 — first minimum wins, which is
    exactly the leftmost tie rule — and subtracting 1 turns it into the offset −1, 0 or +1.
    """
    candidates[0, 0] = np.inf
    candidates[0, 1:] = prev[:-1]
    candidates[1] = prev
    candidates[2, :-1] = prev[1:]
    candidates[2, -1] = np.inf
    choice = np.argmin(candidates, axis=0)
    best = np.take_along_axis(candidates, choice[np.newaxis, :], axis=0)[0]
    return best, (choice - 1).astype(np.int8)


def cumulative_energy(energy: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Fill the full DP tables for **vertical** seams.

    Args:
        energy: ``(H, W)`` real-valued array (not modified).

    Returns:
        ``(M, back)`` where ``M`` is ``float64 (H, W)``: ``M[i, j]`` = minimum total energy of
        any seam from row 0 down to pixel (i, j); and ``back`` is ``int8 (H, W)``:
        ``back[i, j]`` ∈ {−1, 0, +1} is the column offset of the predecessor in row i−1
        (row 0 has no predecessor and stays 0).

    The loop runs over rows because row i needs row i−1 (the DP dependency); the W cells
    inside a row are independent and are computed together with NumPy.
    """
    e = _as_energy(energy)
    h, w = e.shape
    table = np.empty((h, w), dtype=np.float64)
    back = np.zeros((h, w), dtype=np.int8)
    table[0] = e[0]
    candidates = np.empty((3, w), dtype=np.float64)
    for i in range(1, h):
        best, back[i] = _best_predecessors(table[i - 1], candidates)
        table[i] = e[i] + best
    return table, back


def backtrack(last_row: np.ndarray, back: np.ndarray) -> np.ndarray:
    """Recover the seam (one column per row) from the bottom row of ``M`` and the back-pointers.

    Args:
        last_row: ``M[H−1]``, shape ``(W,)``. The seam ends at its leftmost minimum.
        back: ``int8 (H, W)`` back-pointer table from ``cumulative_energy``.

    Returns:
        ``int64`` array of length H: ``cols[i]`` is the seam's column in row i.

    Θ(H) time: one step per row, walking upwards.
    """
    h = back.shape[0]
    cols = np.empty(h, dtype=np.int64)
    cols[h - 1] = int(np.argmin(last_row))
    for i in range(h - 1, 0, -1):
        cols[i - 1] = cols[i] + back[i, cols[i]]
    return cols


def find_vertical_seam(energy: np.ndarray) -> Seam:
    """Minimum-energy **vertical** seam of ``energy`` (H, W) → ``Seam`` with H column indices.

    Same result as ``cumulative_energy`` + ``backtrack``, but keeps only the previous row of
    ``M`` (memory: two float rows + an int8 table instead of a full float64 table).
    """
    e = _as_energy(energy)
    h, w = e.shape
    back = np.zeros((h, w), dtype=np.int8)
    candidates = np.empty((3, w), dtype=np.float64)
    row = e[0].copy()  # M[0] = E[0]
    for i in range(1, h):
        best, back[i] = _best_predecessors(row, candidates)
        row = e[i] + best  # M[i] from M[i−1]; the older row is no longer needed
    cols = backtrack(row, back)
    return Seam(Orientation.VERTICAL, cols, float(row[cols[-1]]))


def find_horizontal_seam(energy: np.ndarray) -> Seam:
    """Minimum-energy **horizontal** seam → ``Seam`` with W row indices (``rows[j]`` per column).

    A horizontal seam of ``E`` is a vertical seam of ``Eᵀ``. ``E.T`` is only a view, so one
    contiguous copy is made to keep the row-by-row DP cache-friendly.
    """
    e = _as_energy(energy)
    vertical = find_vertical_seam(np.ascontiguousarray(e.T))
    return Seam(Orientation.HORIZONTAL, vertical.indices, vertical.energy)


def find_seam(energy: np.ndarray, orientation: Orientation) -> Seam:
    """Shared ``SeamFinder`` entry point (docs/interfaces.md) for the DP algorithm."""
    if orientation is Orientation.VERTICAL:
        return find_vertical_seam(energy)
    if orientation is Orientation.HORIZONTAL:
        return find_horizontal_seam(energy)
    raise TypeError(f"orientation must be an Orientation, got {orientation!r}")
