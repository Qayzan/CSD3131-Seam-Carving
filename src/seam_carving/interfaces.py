"""Shared data types and the seam-finder contract (shared). Owner: FF.

Draft contract — finalised in Milestone 2 after team review (see docs/interfaces.md):

* ``Orientation`` — ``VERTICAL`` (top → bottom, removes one column per row) or
  ``HORIZONTAL`` (left → right, removes one row per column).
* ``Seam`` — orientation, integer coordinate array, total seam energy.
* ``SeamFinder`` — any callable ``find_seam(energy, orientation) -> Seam``. The DP, greedy
  and graph-cut implementations all satisfy it, so the carver, benchmarks and a future UI
  can swap algorithms without code changes.
* ``CarveResult`` — resized image, list of removed seams, timings and optional
  visualisation frames.
"""
