"""Image input/output and colour-space helpers (shared). Owner: FF.

Planned in Milestone 2:

* ``load_image(path) -> np.ndarray`` — reads with OpenCV and converts BGR → RGB, so every
  algorithm in this package sees RGB order. Shape ``(H, W, 3)``, dtype ``uint8``; greyscale
  files load as ``(H, W)``.
* ``save_image(path, image)`` — the reverse conversion (RGB → BGR) before ``cv2.imwrite``.
* ``to_grayscale(image) -> np.ndarray`` — float64 luminance used by the energy functions.
* Input validation (dimensions, dtype, number of channels) with clear error messages.

Convention: arrays are indexed ``image[row, col]`` = ``image[y, x]``; row 0 is the top.
"""
