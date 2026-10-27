"""Shared data types and the seam-finder contract (shared). Owner: FF.

Every seam finder in this package (dynamic programming, greedy, graph cut) speaks the same
"language", defined here:

* ``Orientation`` — ``VERTICAL`` (top → bottom, removes one pixel from every row, so the
  width shrinks by 1) or ``HORIZONTAL`` (left → right, removes one pixel from every column, so
  the height shrinks by 1).
* ``Seam`` — orientation, integer coordinate array, total seam energy.
* ``SeamFinder`` — any callable ``find_seam(energy, orientation) -> Seam``. Because all three
  algorithms satisfy it, the carver, tests, benchmarks and demo can swap algorithms without
  code changes, and the comparison in Q3/Q4/Q5 is fair.
* ``CarveResult`` — the resized image, the removed seams, timings and optional frames.

The full contract (rules every finder must follow) is in ``docs/interfaces.md``.

Coordinates are 0-based and indexed ``array[row, col]``; row 0 is the top row.
"""

from __future__ import annotations

import enum
from dataclasses import dataclass, field
from typing import Protocol, runtime_checkable

import numpy as np

__all__ = ["CarveResult", "Orientation", "Seam", "SeamFinder"]


class Orientation(enum.Enum):
    """Direction of a seam."""

    VERTICAL = "vertical"
    """Runs top → bottom: one pixel per ROW is removed, so the image gets 1 column narrower."""

    HORIZONTAL = "horizontal"
    """Runs left → right: one pixel per COLUMN is removed, so the image gets 1 row shorter."""


@dataclass(frozen=True, eq=False)
class Seam:
    """A connected path of pixels to remove, plus its total energy.

    Attributes:
        orientation: ``Orientation.VERTICAL`` or ``Orientation.HORIZONTAL``.
        indices: 1-D ``int64`` array (stored read-only).

            * VERTICAL: length ``H``; ``indices[i]`` is the **column** removed from row ``i``.
            * HORIZONTAL: length ``W``; ``indices[j]`` is the **row** removed from column ``j``.

        energy: sum of the energy-map values along the seam, using the same energy map the
            finder received (contract rule 3; ``seam_ops.validate_seam`` can check it).

    The constructor copies ``indices`` into a new read-only ``int64`` array, so a seam cannot
    be changed after it is created (the carver keeps seams for visualisation). It checks only
    the *form* of the seam (1-D, integers); whether it fits a particular image is checked by
    ``seam_ops.validate_seam(seam, shape)``.
    """

    orientation: Orientation
    indices: np.ndarray
    energy: float

    def __post_init__(self) -> None:
        if not isinstance(self.orientation, Orientation):
            raise TypeError(
                f"orientation must be an Orientation, got {type(self.orientation).__name__}"
            )
        raw = np.asarray(self.indices)
        if raw.ndim != 1:
            raise ValueError(f"seam indices must be a 1-D array, got shape {raw.shape}")
        if raw.size == 0:
            raise ValueError("seam indices must not be empty")
        # Accept any integer dtype (e.g. intp from np.argmin), but never floats or bools:
        # a float column index such as 2.7 would silently be truncated somewhere later.
        if raw.dtype == np.bool_ or not np.issubdtype(raw.dtype, np.integer):
            raise TypeError(f"seam indices must be integers, got dtype {raw.dtype}")
        indices = raw.astype(np.int64, copy=True)
        indices.setflags(write=False)
        # The dataclass is frozen, so use object.__setattr__ to store the normalised values.
        object.__setattr__(self, "indices", indices)
        object.__setattr__(self, "energy", float(self.energy))

    def __len__(self) -> int:
        """Number of pixels in the seam (H for vertical seams, W for horizontal seams)."""
        return int(self.indices.size)

    def __eq__(self, other: object) -> bool:
        # A dataclass-generated __eq__ would compare the arrays with ``==`` and fail on
        # ``bool(array)``, so compare field by field instead.
        if not isinstance(other, Seam):
            return NotImplemented
        return (
            self.orientation is other.orientation
            and np.array_equal(self.indices, other.indices)
            and self.energy == other.energy
        )

    __hash__ = None  # seams hold arrays, so they are deliberately unhashable

    def coordinates(self) -> tuple[np.ndarray, np.ndarray]:
        """Return ``(rows, cols)`` of every seam pixel, ready for ``array[rows, cols]``.

        Example: a vertical seam ``[1, 1, 2]`` gives rows ``[0, 1, 2]`` and cols ``[1, 1, 2]``.
        """
        positions = np.arange(self.indices.size, dtype=np.int64)
        if self.orientation is Orientation.VERTICAL:
            return positions, self.indices
        return self.indices, positions


@runtime_checkable
class SeamFinder(Protocol):
    """Anything callable as ``finder(energy, orientation) -> Seam``.

    ``energy`` is a ``float64`` array of shape ``(H, W)``. A finder must (see
    docs/interfaces.md): not modify ``energy``; return a valid seam for that shape; set
    ``Seam.energy`` to the sum along the seam; be deterministic; and not use any library
    seam-carving function. Each algorithm module exposes ``find_seam`` with this signature.
    """

    def __call__(self, energy: np.ndarray, orientation: Orientation) -> Seam: ...


@dataclass
class CarveResult:
    """Everything ``carver.carve`` returns.

    Attributes:
        image: the resized image; same dtype and number of channels as the input.
        seams: the removed seams in removal order. Each seam is in the coordinates of the
            image *at the moment it was removed* (the image shrinks after every seam).
        timings: seconds spent in each stage, keys ``"energy"``, ``"find_seam"``,
            ``"remove"`` and ``"total"``.
        frames: optional intermediate images (step-by-step visualisation, bonus 7a), or
            ``None`` when frames were not requested.
    """

    image: np.ndarray
    seams: list[Seam] = field(default_factory=list)
    timings: dict[str, float] = field(default_factory=dict)
    frames: list[np.ndarray] | None = None
