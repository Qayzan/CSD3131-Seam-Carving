"""Energy (importance) maps (shared). Owner: FF.

The energy of a pixel says how much it "stands out". Seam carving removes low-energy pixels
first, so flat areas (sky, walls) disappear before edges and textures.

We use the e1 energy of Avidan & Shamir (2007)::

    e(I) = |dI/dx| + |dI/dy|

* ``I`` is the image converted to **float64 greyscale** (``image_utils.to_grayscale``), so
  differences such as ``200 - 250`` stay negative instead of wrapping around in ``uint8``.
* The derivatives come from the 3×3 **Sobel** operator (``cv2.Sobel``, which the brief allows).
  Sobel also averages across the perpendicular direction, so one noisy pixel matters less
  than with a plain neighbour difference.
* Borders use ``BORDER_REFLECT_101`` (mirror without repeating the edge pixel: ``c b | a b c``).
  Padding with zeros instead would make every border pixel look like a strong edge, and no
  seam would ever touch the image border.

Every seam finder (DP, greedy, graph cut) receives the same energy map, so their seams and
running times can be compared fairly. The carver recomputes the map after every removed seam
(brief requirement R1c).
"""

from __future__ import annotations

import cv2
import numpy as np

from seam_carving.image_utils import to_grayscale

__all__ = ["gradient_energy", "gradient_energy_reference"]

_BORDER = cv2.BORDER_REFLECT_101


def gradient_energy(image: np.ndarray) -> np.ndarray:
    """Return the Sobel gradient-magnitude energy ``|dI/dx| + |dI/dy|`` of ``image``.

    Args:
        image: RGB ``(H, W, 3)`` or greyscale ``(H, W)``; ``uint8`` or float.

    Returns:
        New ``float64`` array of shape ``(H, W)``, every value ``>= 0``. A constant image has
        energy 0 everywhere. Works for any size down to 1×1.

    Cost: Θ(H·W) time and memory — a fixed 3×3 neighbourhood per pixel.
    """
    gray = to_grayscale(image)
    # dx=1 → change from left to right (dI/dx); dy=1 → change from top to bottom (dI/dy).
    dx = cv2.Sobel(gray, cv2.CV_64F, 1, 0, ksize=3, borderType=_BORDER)
    dy = cv2.Sobel(gray, cv2.CV_64F, 0, 1, ksize=3, borderType=_BORDER)
    return np.abs(dx) + np.abs(dy)


def gradient_energy_reference(image: np.ndarray) -> np.ndarray:
    """Same energy as ``gradient_energy``, written out in plain NumPy (no OpenCV).

    This is the *independent check* for ``gradient_energy``: the tests require the two to agree
    on random images, so a wrong OpenCV flag (kernel size, border mode, data type) would be
    caught. It also shows exactly what the Sobel filter computes, which the report can quote.

    The 3×3 Sobel kernels (applied as correlation, ``*`` = centre pixel)::

        d/dx:  -1  0  +1        d/dy:  -1  -2  -1
               -2  *  +2                0   *   0
               -1  0  +1               +1  +2  +1

    ``np.pad(..., mode="reflect")`` is the same mirror rule as OpenCV's ``BORDER_REFLECT_101``.
    """
    gray = to_grayscale(image)
    p = np.pad(gray, 1, mode="reflect")
    # p[r:r+H, c:c+W] is the image shifted so that it lines up with neighbour (r-1, c-1).
    h, w = gray.shape

    def shifted(r: int, c: int) -> np.ndarray:
        return p[r : r + h, c : c + w]

    dx = (
        (shifted(0, 2) - shifted(0, 0))
        + 2.0 * (shifted(1, 2) - shifted(1, 0))
        + (shifted(2, 2) - shifted(2, 0))
    )
    dy = (
        (shifted(2, 0) - shifted(0, 0))
        + 2.0 * (shifted(2, 1) - shifted(0, 1))
        + (shifted(2, 2) - shifted(0, 2))
    )
    return np.abs(dx) + np.abs(dy)
