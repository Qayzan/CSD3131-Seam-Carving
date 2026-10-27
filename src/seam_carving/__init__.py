"""Content-aware image resizing (seam carving) — CSD3131 Assignment 2, Team T11.

Package layout (see docs/algorithm_design.md for the full architecture):

========================  ======================================  =====================
Module                    Purpose                                 Owner
========================  ======================================  =====================
``image_utils``           Image loading/saving, colour handling   FF (shared)
``energy``                Energy maps (gradient / Sobel)          FF (shared)
``interfaces``            Shared seam / result types              FF (shared)
``seam_ops``              Seam validation, removal, highlighting  FF (shared)
``carver``                Repeated resizing with any seam finder  FF (shared)
``dynamic_programming``   Question 1 — DP seam finder             FF
``greedy``                Question 3 — greedy seam finder         HM
``graph_cut``             Question 4 — min-cut seam finder        WN
========================  ======================================  =====================

Milestone 2 implements the shared utilities (``image_utils``, ``energy``, ``interfaces``,
``seam_ops``). ``carver`` and ``dynamic_programming`` follow in Milestone 3; ``greedy`` and
``graph_cut`` are written by their owners on their own feature branches.
"""

__version__ = "0.1.0"
