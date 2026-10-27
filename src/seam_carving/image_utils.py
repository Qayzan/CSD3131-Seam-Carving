"""Image input/output and colour-space helpers (shared). Owner: FF.

Conventions used by the whole package (see docs/interfaces.md):

* Images are NumPy arrays indexed ``image[row, col]`` (= ``image[y, x]``); row 0 is the top.
* Colour images are **RGB**, ``uint8``, shape ``(H, W, 3)``. Greyscale images are ``uint8``,
  shape ``(H, W)``. OpenCV works in BGR, so the conversion happens only here, at load/save.
* ``to_grayscale`` returns ``float64`` so later arithmetic never overflows: with ``uint8``,
  ``200 - 250`` would wrap around to 206 instead of giving -50.

Files are read and written through ``np.fromfile`` + ``cv2.imdecode`` (and ``cv2.imencode`` +
``tofile``) instead of ``cv2.imread``/``cv2.imwrite``, because the latter fail silently on
Windows paths containing non-ASCII characters (common in OneDrive folders).
"""

from __future__ import annotations

from os import PathLike
from pathlib import Path

import cv2
import numpy as np

__all__ = ["LUMA_WEIGHTS", "load_image", "save_image", "to_grayscale", "validate_image"]

# ITU-R BT.601 luma weights for R, G, B — the same weights OpenCV's RGB2GRAY uses.
LUMA_WEIGHTS = np.array([0.299, 0.587, 0.114], dtype=np.float64)


def validate_image(image: np.ndarray, *, name: str = "image") -> np.ndarray:
    """Check that ``image`` follows the package's image convention and return it unchanged.

    Accepted: a NumPy array of shape ``(H, W)`` (greyscale) or ``(H, W, 3)`` (RGB) with
    ``H >= 1`` and ``W >= 1``, and dtype ``uint8`` or a floating-point type (float images are
    allowed so energy functions can be tested on hand-written matrices).

    Raises:
        TypeError: not a NumPy array, or an unsupported dtype (bool, signed/other integers,
            complex, object).
        ValueError: wrong number of dimensions or channels, or an empty image.
    """
    if not isinstance(image, np.ndarray):
        raise TypeError(f"{name} must be a numpy.ndarray, got {type(image).__name__}")
    if image.ndim not in (2, 3):
        raise ValueError(f"{name} must have shape (H, W) or (H, W, 3), got {image.shape}")
    if image.ndim == 3 and image.shape[2] != 3:
        raise ValueError(
            f"{name} must have 3 channels (RGB) when 3-D, got {image.shape[2]} "
            f"(shape {image.shape}); drop the alpha channel first"
        )
    if image.shape[0] < 1 or image.shape[1] < 1:
        raise ValueError(f"{name} must be at least 1x1, got shape {image.shape}")
    if not (image.dtype == np.uint8 or np.issubdtype(image.dtype, np.floating)):
        raise TypeError(f"{name} must have dtype uint8 or float, got {image.dtype}")
    return image


def to_grayscale(image: np.ndarray) -> np.ndarray:
    """Return the luminance of ``image`` as ``float64``, shape ``(H, W)``.

    * RGB ``(H, W, 3)``: ``0.299 R + 0.587 G + 0.114 B`` (BT.601), computed in float64 with no
      rounding (``cv2.cvtColor`` would round back to ``uint8``).
    * Greyscale ``(H, W)``: converted to float64, values unchanged.

    The input is never modified; a new array is always returned.
    """
    validate_image(image)
    if image.ndim == 2:
        return image.astype(np.float64)  # astype copies, so callers can't alias the input
    return image.astype(np.float64) @ LUMA_WEIGHTS


def load_image(path: str | PathLike[str]) -> np.ndarray:
    """Read an image file into the package convention.

    Returns:
        RGB ``uint8`` array ``(H, W, 3)`` for colour files, or ``uint8`` ``(H, W)`` for
        greyscale files. An alpha channel is dropped (seam carving ignores transparency) and
        16-bit images are scaled down to 8 bits.

    Raises:
        FileNotFoundError: ``path`` does not exist.
        ValueError: the file exists but OpenCV cannot decode it as an image.
    """
    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError(f"image file not found: {path}")
    data = np.fromfile(path, dtype=np.uint8)
    image = cv2.imdecode(data, cv2.IMREAD_UNCHANGED) if data.size else None
    if image is None:
        raise ValueError(f"could not decode image file: {path}")

    if image.dtype == np.uint16:
        # 0..65535 → 0..255; 257 = 65535 / 255 maps both ends exactly.
        image = np.round(image / 257.0).astype(np.uint8)
    elif image.dtype != np.uint8:
        raise ValueError(f"unsupported image depth {image.dtype} in {path}")

    if image.ndim == 3 and image.shape[2] == 1:
        image = image[:, :, 0]
    if image.ndim == 2:
        return np.ascontiguousarray(image)
    if image.shape[2] == 4:
        return cv2.cvtColor(image, cv2.COLOR_BGRA2RGB)
    if image.shape[2] == 3:
        return cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    raise ValueError(f"unsupported number of channels {image.shape[2]} in {path}")


def save_image(path: str | PathLike[str], image: np.ndarray) -> Path:
    """Write an RGB (or greyscale) ``uint8`` image; the format comes from the file extension.

    The parent folder is created if needed. Returns the path written.

    Raises:
        TypeError / ValueError: ``image`` does not follow the convention, or is not ``uint8``
            (convert float images explicitly, e.g. with ``np.clip(x, 0, 255).astype(np.uint8)``,
            so no silent rounding happens here).
        ValueError: the extension is missing or OpenCV cannot encode that format.
    """
    validate_image(image)
    if image.dtype != np.uint8:
        raise TypeError(f"save_image expects a uint8 image, got {image.dtype}")
    path = Path(path)
    if not path.suffix:
        raise ValueError(f"cannot choose an image format without a file extension: {path}")
    bgr = image if image.ndim == 2 else cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
    try:
        ok, buffer = cv2.imencode(path.suffix, bgr)
    except cv2.error as exc:  # OpenCV raises (rather than returns False) for unknown formats
        raise ValueError(f"OpenCV could not encode an image as {path.suffix!r}") from exc
    if not ok:
        raise ValueError(f"OpenCV could not encode an image as {path.suffix!r}")
    path.parent.mkdir(parents=True, exist_ok=True)
    buffer.tofile(path)
    return path
