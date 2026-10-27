"""Operations on seams that every algorithm needs (shared). Owner: FF.

Planned in Milestone 2/3:

* ``validate_seam(seam, shape)`` — one pixel per row (vertical) or column (horizontal),
  indices in range, neighbouring pixels at most one apart (8-connected).
* ``seam_energy(energy, seam)`` — sum of the energies along a seam.
* ``remove_seam(image, seam)`` — returns a new array one column (or row) smaller; all other
  pixels keep their values and order; dtype is preserved.
* ``highlight_seam(image, seam, colour)`` — copy of the image with the seam drawn on it.
"""
