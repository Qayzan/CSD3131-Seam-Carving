"""Repeated resizing driven by any seam finder (shared). Owner: FF.

``carve`` shrinks an image to a target size by removing one seam at a time. Before **every**
seam it recomputes the energy map (brief requirement R1c): removing a seam brings pixels that
were apart next to each other, so their gradients — and the next best seam — change.

The seam finder is a parameter, so the DP (Q1), greedy (Q3) and graph-cut (Q4) finders run
through exactly the same loop, energy function, removal code and timing. That keeps the
comparisons in Q3/Q4/Q5 fair.

Loop for one seam::

    E    = energy_fn(image)            # float64 (H, W), handed to the finder read-only
    seam = finder(E, orientation)      # timed on its own: timings["find_seam"]
    validate_seam(seam, E.shape, energy=E)   # a broken finder fails loudly here
    image = remove_seam(image, seam)

Order of removals when both dimensions shrink (design §2.8): ``"width_first"`` (default:
all vertical seams, then all horizontal), ``"height_first"``, or ``"alternate"`` (vertical,
horizontal, vertical, … while both still need shrinking). Shrinking only — no enlargement.
"""

from __future__ import annotations

import time
from collections.abc import Callable, Iterator

import numpy as np

from seam_carving.energy import gradient_energy
from seam_carving.image_utils import validate_image
from seam_carving.interfaces import CarveResult, Orientation, Seam, SeamFinder
from seam_carving.seam_ops import remove_seam, seam_mask, validate_seam

__all__ = ["ORDERS", "carve", "removal_plan", "seams_in_original_coordinates"]

ORDERS = ("width_first", "height_first", "alternate")


def _check_target(name: str, value: object, current: int) -> int:
    # bool is a subclass of int in Python; True as a "height" is almost certainly a bug.
    if isinstance(value, bool) or not isinstance(value, (int, np.integer)):
        raise TypeError(f"{name} must be an integer, got {type(value).__name__}")
    value = int(value)
    if value < 1:
        raise ValueError(f"{name} must be at least 1, got {value}")
    if value > current:
        raise ValueError(
            f"{name} = {value} is larger than the current size {current}; "
            "carve() only shrinks images (seam insertion is out of scope)"
        )
    return value


def removal_plan(n_vertical: int, n_horizontal: int, order: str) -> Iterator[Orientation]:
    """Yield the orientation of each seam to remove, in order.

    ``n_vertical`` = columns to remove, ``n_horizontal`` = rows to remove.
    """
    if order not in ORDERS:
        raise ValueError(f"order must be one of {ORDERS}, got {order!r}")
    v, h = Orientation.VERTICAL, Orientation.HORIZONTAL
    if order == "width_first":
        yield from [v] * n_vertical + [h] * n_horizontal
    elif order == "height_first":
        yield from [h] * n_horizontal + [v] * n_vertical
    else:
        for k in range(max(n_vertical, n_horizontal)):
            if k < n_vertical:
                yield v
            if k < n_horizontal:
                yield h


def carve(
    image: np.ndarray,
    target_height: int,
    target_width: int,
    finder: SeamFinder | None = None,
    order: str = "width_first",
    keep_frames: bool = False,
    *,
    energy_fn: Callable[[np.ndarray], np.ndarray] = gradient_energy,
) -> CarveResult:
    """Shrink ``image`` to ``(target_height, target_width)`` by removing minimum-energy seams.

    Args:
        image: RGB ``uint8 (H, W, 3)`` or greyscale ``(H, W)``. Not modified.
        target_height, target_width: the size to reach; ``1 <= target <= current``.
        finder: any ``SeamFinder`` (``find_seam(energy, orientation) -> Seam``). Default: the
            dynamic-programming finder of Question 1.
        order: ``"width_first"`` (default), ``"height_first"`` or ``"alternate"``.
        keep_frames: if True, ``result.frames`` holds the input and the image after every
            removal (``len = seams + 1``) for step-by-step visualisation.
        energy_fn: energy function, recomputed before every seam. Default
            ``energy.gradient_energy``. (A parameter so tests can count the calls and the
            report can try other energies.)

    Returns:
        ``CarveResult`` with the resized image, the seams in removal order (each in the
        coordinates of the image at that step) and timings in seconds: ``"energy"``,
        ``"find_seam"``, ``"remove"`` (each summed over all seams) and ``"total"``.

    Raises:
        TypeError / ValueError: invalid image or target size, unknown ``order``.
        ValueError: the finder returned an invalid seam, a wrong ``Seam.energy``, or tried to
            modify the energy map (it is passed read-only).
    """
    start = time.perf_counter()
    validate_image(image)
    if finder is None:
        from seam_carving.dynamic_programming import find_seam as finder
    h, w = image.shape[:2]
    target_height = _check_target("target_height", target_height, h)
    target_width = _check_target("target_width", target_width, w)
    plan = list(removal_plan(w - target_width, h - target_height, order))

    timings = {"energy": 0.0, "find_seam": 0.0, "remove": 0.0}
    seams: list[Seam] = []
    frames: list[np.ndarray] | None = [image.copy()] if keep_frames else None
    current = image
    for orientation in plan:
        t0 = time.perf_counter()
        energy = energy_fn(current)
        energy.setflags(write=False)  # contract rule 1: finders must not modify the energy
        t1 = time.perf_counter()
        seam = finder(energy, orientation)
        t2 = time.perf_counter()
        if seam.orientation is not orientation:
            raise ValueError(
                f"finder returned a {seam.orientation.value} seam, asked for {orientation.value}"
            )
        validate_seam(seam, energy.shape, energy=energy)
        current = remove_seam(current, seam)
        t3 = time.perf_counter()
        timings["energy"] += t1 - t0
        timings["find_seam"] += t2 - t1
        timings["remove"] += t3 - t2
        seams.append(seam)
        if frames is not None:
            frames.append(current)

    result_image = current if plan else image.copy()
    timings["total"] = time.perf_counter() - start
    return CarveResult(image=result_image, seams=seams, timings=timings, frames=frames)


def seams_in_original_coordinates(
    shape: tuple[int, ...], seams: list[Seam]
) -> list[tuple[np.ndarray, np.ndarray]]:
    """Map every removed seam back to pixel positions in the ORIGINAL image.

    Each seam in a ``CarveResult`` is stored in the coordinates of the image it was removed
    from, which has already lost earlier seams. To draw all seams on the original picture
    (bonus 7a) we replay the removals on two index maps that remember, for every surviving
    pixel, its original row and column.

    Args:
        shape: the original image shape ``(H, W[, C])``.
        seams: seams in removal order (``CarveResult.seams``).

    Returns:
        One ``(rows, cols)`` pair of ``int64`` arrays per seam, in original coordinates.
    """
    h, w = int(shape[0]), int(shape[1])
    row_of, col_of = np.indices((h, w), dtype=np.int64)
    positions = []
    for seam in seams:
        mask = seam_mask(row_of.shape, seam)
        positions.append((row_of[mask], col_of[mask]))
        keep = ~mask
        if seam.orientation is Orientation.VERTICAL:
            new_shape = (row_of.shape[0], row_of.shape[1] - 1)
            row_of, col_of = row_of[keep].reshape(new_shape), col_of[keep].reshape(new_shape)
        else:
            new_shape = (row_of.shape[0] - 1, row_of.shape[1])
            # Remove along columns: transpose so that the mask selection is row-major per column.
            row_of = row_of.T[keep.T].reshape(new_shape[::-1]).T
            col_of = col_of.T[keep.T].reshape(new_shape[::-1]).T
    return positions
