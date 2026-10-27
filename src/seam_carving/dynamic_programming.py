"""Question 1 — minimum-energy seams by Dynamic Programming. Owner: FF (Fauzan Fauzy).

Planned in Milestone 3 (design: docs/algorithm_design.md, section "Question 1"):

* ``cumulative_energy(energy) -> (M, back)`` — bottom-up table
  ``M[i, j] = E[i, j] + min(M[i-1, j-1], M[i-1, j], M[i-1, j+1])`` over valid columns,
  plus a back-pointer table recording which predecessor was chosen.
* ``backtrack(M, back) -> np.ndarray`` — seam columns from the bottom row to the top.
* ``find_vertical_seam`` / ``find_horizontal_seam`` / ``find_seam(energy, orientation)``;
  horizontal seams reuse the vertical code on the transposed energy map.
"""
