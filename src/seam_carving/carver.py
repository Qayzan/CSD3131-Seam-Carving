"""Repeated resizing driven by any seam finder (shared). Owner: FF.

Planned in Milestone 3:

* ``carve(image, target_height, target_width, finder, ...) -> CarveResult`` — validates the
  target size (shrinking only), then loops: recompute the energy map → find a seam → remove
  it, until the target size is reached. The energy map is recalculated after every removal,
  as the brief requires.
* The order of vertical and horizontal removals when both dimensions shrink is documented in
  docs/algorithm_design.md.
* Optional capture of intermediate frames for step-by-step visualisation (bonus item 7a).
"""
