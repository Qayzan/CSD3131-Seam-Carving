"""Energy (importance) maps (shared). Owner: FF.

Planned in Milestone 2:

* ``gradient_energy(image) -> np.ndarray`` — e1 energy from Avidan & Shamir (2007):
  ``|dI/dx| + |dI/dy|`` computed with the 3×3 Sobel operator (``cv2.Sobel``, allowed by the
  brief) on a float64 greyscale image. Output: shape ``(H, W)``, dtype ``float64``, all values
  ``>= 0``. Borders use OpenCV's default reflection (``BORDER_REFLECT_101``), so edge pixels
  are not artificially high-energy.
* A small pure-NumPy reference implementation used only to cross-check the OpenCV version in
  tests.

Every seam finder (DP, greedy, graph cut) consumes the same energy map so their results are
directly comparable.
"""
