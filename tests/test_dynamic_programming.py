"""Tests for Question 1: the dynamic-programming seam finder.

Strategy: (1) the hand-worked 4x5 example from docs/algorithm_design.md, (2) two independent
oracles — exhaustive search over every seam, and the recurrence written as plain loops —
on hundreds of random matrices, (3) targeted edge cases, (4) the shared contract.
"""

import numpy as np
import pytest

from helpers import (
    WORKED_ENERGY,
    all_vertical_seams,
    brute_force_find_seam,
    reference_cumulative_energy,
)
from seam_carving.dynamic_programming import (
    backtrack,
    cumulative_energy,
    find_horizontal_seam,
    find_seam,
    find_vertical_seam,
)
from seam_carving.interfaces import Orientation, Seam, SeamFinder
from seam_carving.seam_ops import seam_energy, validate_seam

V, H = Orientation.VERTICAL, Orientation.HORIZONTAL

# --- 1. The worked example (docs/algorithm_design.md §2.5) ----------------------------------


def test_worked_example_table_matches_the_design_document():
    table, back = cumulative_energy(WORKED_ENERGY)
    expected = [
        [3, 1, 4, 1, 5],
        [10, 3, 7, 6, 4],
        [8, 11, 4, 13, 11],
        [10, 10, 9, 7, 16],
    ]
    np.testing.assert_array_equal(table, expected)
    assert table.dtype == np.float64 and back.dtype == np.int8
    # Row 1, column 2: predecessors 1 (col 1), 4 (col 2), 1 (col 3) tie → leftmost (−1) wins.
    assert back[1, 2] == -1
    assert np.all(back[0] == 0)  # row 0 has no predecessor


def test_worked_example_seam():
    seam = find_vertical_seam(WORKED_ENERGY)
    assert seam.orientation is V
    assert seam.indices.tolist() == [1, 1, 2, 3]
    assert seam.energy == 7.0  # 1 + 2 + 1 + 3


def test_worked_example_has_95_seams_and_7_is_the_minimum():
    seams = list(all_vertical_seams(4, 5))
    assert len(seams) == 95
    costs = [sum(WORKED_ENERGY[i, c] for i, c in enumerate(s)) for s in seams]
    assert min(costs) == 7.0
    assert seams[int(np.argmin(costs))] == (1, 1, 2, 3)


def test_backtrack_on_the_worked_example():
    table, back = cumulative_energy(WORKED_ENERGY)
    assert backtrack(table[-1], back).tolist() == [1, 1, 2, 3]


# --- 2. Independent oracles on random inputs -------------------------------------------------


@pytest.mark.parametrize("seed", range(150))
def test_matches_exhaustive_search_exactly_on_integer_energies(seed):
    """Integer energies give many exact ties, so this also checks the leftmost tie rule."""
    rng = np.random.default_rng(seed)
    h, w = rng.integers(1, 7, size=2)
    energy = rng.integers(0, 6, size=(h, w)).astype(np.float64)  # values 0..5 → lots of ties
    for orientation in (V, H):
        assert find_seam(energy, orientation) == brute_force_find_seam(energy, orientation)


@pytest.mark.parametrize("seed", range(60))
def test_matches_exhaustive_search_on_random_real_energies(seed):
    rng = np.random.default_rng(10_000 + seed)
    h, w = rng.integers(1, 7, size=2)
    energy = rng.random((h, w)) * 1000
    for orientation in (V, H):
        seam = find_seam(energy, orientation)
        oracle = brute_force_find_seam(energy, orientation)
        assert seam.energy == pytest.approx(oracle.energy, rel=1e-12)
        validate_seam(seam, energy.shape, energy=energy)


@pytest.mark.parametrize("shape", [(1, 1), (1, 9), (9, 1), (5, 5), (12, 31), (31, 12)])
def test_table_matches_plain_loop_reference(rng, shape):
    energy = rng.random(shape) * 255
    table, back = cumulative_energy(energy)
    ref_table, ref_back = reference_cumulative_energy(energy)
    np.testing.assert_array_equal(table, ref_table)
    np.testing.assert_array_equal(back, ref_back)


@pytest.mark.parametrize("shape", [(1, 1), (6, 1), (1, 6), (40, 57), (57, 40)])
def test_memory_saving_finder_agrees_with_full_table(rng, shape):
    """find_vertical_seam keeps two rows of M; cumulative_energy keeps all of them."""
    energy = rng.integers(0, 50, size=shape).astype(np.float64)
    table, back = cumulative_energy(energy)
    seam = find_vertical_seam(energy)
    assert seam.indices.tolist() == backtrack(table[-1], back).tolist()
    assert seam.energy == table[-1].min()


# --- 3. Edge cases ---------------------------------------------------------------------------


def test_uniform_energy_gives_column_zero_straight_down():
    seam = find_vertical_seam(np.ones((6, 4)))
    assert seam.indices.tolist() == [0] * 6 and seam.energy == 6.0


def test_seam_passes_through_the_gap_in_a_wall():
    energy = np.zeros((5, 7))
    energy[2, :] = 100.0  # a high-energy wall across row 2 …
    energy[2, 5] = 0.0  # … with one gap at column 5
    seam = find_vertical_seam(energy)
    assert seam.indices[2] == 5 and seam.energy == 0.0
    validate_seam(seam, energy.shape)


@pytest.mark.parametrize("side", ["left", "right"])
def test_seams_along_the_border(side):
    energy = np.full((5, 6), 9.0)
    column = 0 if side == "left" else 5
    energy[:, column] = 1.0  # the only cheap path hugs the border (tests the +inf padding)
    assert find_vertical_seam(energy).indices.tolist() == [column] * 5


def test_diagonal_valley():
    energy = np.full((5, 5), 9.0)
    energy[np.arange(5), np.arange(5)] = 0.0
    assert find_vertical_seam(energy).indices.tolist() == [0, 1, 2, 3, 4]


def test_cheap_pixel_that_cannot_be_reached_is_not_used():
    # The seam may move only one column per row, so it cannot jump to the 0 in the corner.
    energy = np.array([[0, 5, 5, 5], [5, 5, 5, 5], [5, 5, 5, 0]], dtype=np.float64)
    seam = find_vertical_seam(energy)
    assert seam.energy == 10.0
    assert np.all(np.abs(np.diff(seam.indices)) <= 1)


@pytest.mark.parametrize("shape", [(1, 1), (1, 7), (7, 1)])
def test_degenerate_shapes(shape):
    energy = np.arange(np.prod(shape), dtype=np.float64).reshape(shape)
    for orientation in (V, H):
        seam = find_seam(energy, orientation)
        validate_seam(seam, shape, energy=energy)


def test_one_row_image_vertical_seam_is_the_cheapest_pixel():
    seam = find_vertical_seam(np.array([[4.0, 2.0, 2.0, 7.0]]))
    assert seam.indices.tolist() == [1] and seam.energy == 2.0  # leftmost of the tie


def test_horizontal_seam_is_the_vertical_seam_of_the_transpose(rng):
    energy = rng.random((4, 7))  # non-square, so an H/W mix-up fails loudly
    horizontal = find_horizontal_seam(energy)
    vertical_of_transpose = find_vertical_seam(energy.T)
    assert horizontal.orientation is H
    assert len(horizontal) == 7
    assert horizontal.indices.tolist() == vertical_of_transpose.indices.tolist()
    assert horizontal.energy == vertical_of_transpose.energy


@pytest.mark.parametrize("shape", [(4, 7), (7, 4)])
def test_seam_lengths_follow_the_contract(rng, shape):
    energy = rng.random(shape)
    assert len(find_seam(energy, V)) == shape[0]  # one column per row
    assert len(find_seam(energy, H)) == shape[1]  # one row per column


def test_negative_energies_still_give_the_minimum():
    energy = np.array([[0.0, -5.0, 0.0], [0.0, 0.0, -5.0]])
    seam = find_vertical_seam(energy)
    assert seam.energy == -10.0 and seam.indices.tolist() == [1, 2]


def test_integer_energy_arrays_are_accepted():
    seam = find_vertical_seam(WORKED_ENERGY.astype(np.int32))
    assert seam.indices.tolist() == [1, 1, 2, 3]


@pytest.mark.parametrize(
    ("energy", "error"),
    [
        (np.zeros(5), ValueError),  # 1-D
        (np.zeros((2, 2, 3)), ValueError),  # an image, not an energy map
        (np.zeros((0, 3)), ValueError),
        (np.array([[1.0, np.nan]]), ValueError),
        (np.array([[1.0, np.inf]]), ValueError),
        (np.zeros((2, 2), dtype=bool), TypeError),
        ([[1.0, 2.0]], TypeError),  # a list
    ],
)
def test_invalid_energy_maps_are_rejected(energy, error):
    with pytest.raises(error):
        find_vertical_seam(energy)


def test_unknown_orientation_is_rejected():
    with pytest.raises(TypeError):
        find_seam(np.ones((2, 2)), "vertical")


# --- 4. The shared contract (docs/interfaces.md) ---------------------------------------------


def test_find_seam_is_a_seam_finder():
    assert isinstance(find_seam, SeamFinder)


def test_energy_is_not_modified_and_read_only_input_works(rng):
    energy = rng.random((8, 9))
    before = energy.copy()
    energy.setflags(write=False)  # the carver passes the map read-only
    for orientation in (V, H):
        find_seam(energy, orientation)
    np.testing.assert_array_equal(energy, before)


def test_reported_energy_equals_the_sum_along_the_seam(rng):
    energy = rng.random((30, 40)) * 255
    for orientation in (V, H):
        seam = find_seam(energy, orientation)
        assert isinstance(seam, Seam)
        assert seam.energy == pytest.approx(seam_energy(energy, seam), rel=1e-12)
        assert seam.indices.dtype == np.int64


def test_deterministic(rng):
    energy = rng.integers(0, 3, size=(20, 20)).astype(np.float64)  # many ties
    first = find_seam(energy, V)
    assert all(find_seam(energy, V) == first for _ in range(5))
