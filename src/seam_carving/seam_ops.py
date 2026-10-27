"""Operations on seams that every algorithm needs (shared). Owner: FF.

* ``validate_seam`` — is this a legal seam for an image of this size? (The carver calls it
  after every finder call, so a buggy finder fails loudly instead of corrupting the image.)
* ``seam_energy`` — total energy along a seam.
* ``seam_mask`` — boolean mask that is ``True`` exactly on the seam pixels.
* ``remove_seam`` — the image with the seam taken out (one column or row smaller).
* ``highlight_seam`` — a copy of the image with the seam painted in a colour (for figures).

Seam coordinates follow docs/interfaces.md: a VERTICAL seam stores one column per row
(length ``H``), a HORIZONTAL seam stores one row per column (length ``W``).
"""

from __future__ import annotations

import numpy as np

from seam_carving.image_utils import validate_image
from seam_carving.interfaces import Orientation, Seam

__all__ = ["highlight_seam", "remove_seam", "seam_energy", "seam_mask", "validate_seam"]


def _height_width(shape: tuple[int, ...]) -> tuple[int, int]:
    if len(shape) < 2:
        raise ValueError(f"expected an image or energy shape (H, W[, C]), got {shape}")
    return int(shape[0]), int(shape[1])


def validate_seam(
    seam: Seam,
    shape: tuple[int, ...],
    *,
    energy: np.ndarray | None = None,
    rtol: float = 1e-9,
    atol: float = 1e-9,
) -> None:
    """Raise ``ValueError`` unless ``seam`` is a legal seam for an image of ``shape``.

    Checks, in order:

    1. ``seam`` is a ``Seam``.
    2. Length: ``H`` for a vertical seam, ``W`` for a horizontal one.
    3. Every index is inside the image (``0 <= index < W`` for vertical, ``< H`` for horizontal).
    4. Connectivity: neighbouring entries differ by at most 1 (8-connected path).
    5. Only if ``energy`` is given: ``seam.energy`` equals the sum of ``energy`` along the seam,
       within ``rtol``/``atol`` (contract rule 3).

    Args:
        seam: the seam to check.
        shape: ``image.shape`` or ``energy.shape``; only the first two numbers ``(H, W)`` matter.
        energy: optional ``(H, W)`` energy map the seam was computed from.

    Returns ``None`` when the seam is valid, so it can be used as a plain assertion.
    """
    if not isinstance(seam, Seam):
        raise TypeError(f"expected a Seam, got {type(seam).__name__}")
    h, w = _height_width(shape)
    idx = seam.indices
    if seam.orientation is Orientation.VERTICAL:
        length, limit, along, across = h, w, "row", "column"
    else:
        length, limit, along, across = w, h, "column", "row"
    name = seam.orientation.value

    if idx.size != length:
        raise ValueError(
            f"{name} seam for an image of size {h}x{w} must have {length} entries "
            f"(one per {along}), got {idx.size}"
        )
    bad = np.flatnonzero((idx < 0) | (idx >= limit))
    if bad.size:
        k = int(bad[0])
        raise ValueError(
            f"{name} seam leaves the image: {along} {k} has {across} {int(idx[k])}, "
            f"valid {across}s are 0..{limit - 1}"
        )
    jumps = np.flatnonzero(np.abs(np.diff(idx)) > 1)
    if jumps.size:
        k = int(jumps[0])
        raise ValueError(
            f"{name} seam is not connected: {along} {k} -> {k + 1} jumps from {across} "
            f"{int(idx[k])} to {int(idx[k + 1])} (neighbours may differ by at most 1)"
        )
    if energy is not None:
        if energy.shape[:2] != (h, w):
            raise ValueError(f"energy map shape {energy.shape} does not match {(h, w)}")
        expected = seam_energy(energy, seam)
        if not np.isclose(seam.energy, expected, rtol=rtol, atol=atol):
            raise ValueError(
                f"seam.energy = {seam.energy!r} but the energy along the seam is {expected!r}"
            )


def seam_energy(energy: np.ndarray, seam: Seam) -> float:
    """Return the sum of ``energy`` over the seam's pixels (as a Python ``float``).

    ``energy`` has shape ``(H, W)``. The seam must fit it (call ``validate_seam`` first if
    unsure; out-of-range indices raise ``IndexError`` here).
    """
    rows, cols = seam.coordinates()
    return float(np.sum(energy[rows, cols], dtype=np.float64))


def seam_mask(shape: tuple[int, ...], seam: Seam) -> np.ndarray:
    """Boolean ``(H, W)`` mask: ``True`` on the seam pixels, ``False`` everywhere else.

    Validates the seam first, so the mask always has exactly one ``True`` per row (vertical)
    or per column (horizontal).
    """
    validate_seam(seam, shape)
    h, w = _height_width(shape)
    mask = np.zeros((h, w), dtype=bool)
    rows, cols = seam.coordinates()
    mask[rows, cols] = True
    return mask


def remove_seam(image: np.ndarray, seam: Seam) -> np.ndarray:
    """Return a new image with the seam's pixels removed.

    * VERTICAL seam on ``(H, W[, C])`` → ``(H, W-1[, C])``: in every row the pixels right of
      the seam shift one place left.
    * HORIZONTAL seam → ``(H-1, W[, C])``: in every column the pixels below the seam shift up.

    The dtype, the number of channels, and the values and order of all other pixels are kept.
    The input image is not modified.

    How: one boolean mask selects every pixel *except* the seam; because each row keeps
    exactly ``W-1`` pixels, the selected values reshape straight into the smaller image.
    Horizontal seams reuse the same code on the transposed image.

    Raises:
        ValueError: the seam does not fit the image, or the image is already 1 pixel wide
            (vertical) / 1 pixel tall (horizontal), so nothing would be left.
    """
    validate_image(image)
    validate_seam(seam, image.shape)
    if seam.orientation is Orientation.HORIZONTAL:
        if image.shape[0] == 1:
            raise ValueError("cannot remove a horizontal seam from an image that is 1 pixel tall")
        axes = (1, 0, 2) if image.ndim == 3 else (1, 0)
        flipped = Seam(Orientation.VERTICAL, seam.indices, seam.energy)
        return np.ascontiguousarray(remove_seam(image.transpose(axes), flipped).transpose(axes))

    h, w = image.shape[:2]
    if w == 1:
        raise ValueError("cannot remove a vertical seam from an image that is 1 pixel wide")
    keep = ~seam_mask((h, w), seam)
    # image[keep] lists the kept pixels row by row (row-major order), W-1 per row.
    return image[keep].reshape((h, w - 1) + image.shape[2:])


def highlight_seam(
    image: np.ndarray, seam: Seam, colour: tuple[int, int, int] = (255, 0, 0)
) -> np.ndarray:
    """Return an RGB ``uint8`` copy of ``image`` with the seam's pixels painted ``colour``.

    Greyscale input is expanded to RGB so the colour shows. Float input is clipped to
    ``0..255``. The default colour is red; the input image is not modified.
    """
    validate_image(image)
    validate_seam(seam, image.shape)
    rgb = image if image.ndim == 3 else np.repeat(image[:, :, np.newaxis], 3, axis=2)
    out = np.clip(rgb, 0, 255).astype(np.uint8)  # astype always copies
    rows, cols = seam.coordinates()
    out[rows, cols] = np.asarray(colour, dtype=np.uint8)
    return out
