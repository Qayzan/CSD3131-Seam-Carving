"""Tests for seam_carving.seam_ops: validation, energy, masks, removal and highlighting."""

import numpy as np
import pytest

from helpers import WORKED_ENERGY, random_seam
from seam_carving.interfaces import Orientation, Seam
from seam_carving.seam_ops import (
    highlight_seam,
    remove_seam,
    seam_energy,
    seam_mask,
    validate_seam,
)

V, H = Orientation.VERTICAL, Orientation.HORIZONTAL

# --- validate_seam ---------------------------------------------------------------------


@pytest.mark.parametrize(
    "seam",
    [
        Seam(V, [1, 1, 2, 3], 7.0),  # the worked example's optimal seam
        Seam(V, [0, 0, 0, 0], 0.0),  # along the left border
        Seam(V, [4, 4, 4, 4], 0.0),  # along the right border
        Seam(V, [0, 1, 2, 3], 0.0),  # diagonal
        Seam(H, [0, 1, 2, 3, 3], 0.0),
        Seam(H, [3, 3, 3, 3, 3], 0.0),
    ],
)
def test_valid_seams_pass(seam):
    assert validate_seam(seam, (4, 5)) is None
    assert validate_seam(seam, (4, 5, 3)) is None  # an image shape works too


@pytest.mark.parametrize(
    ("seam", "message"),
    [
        (Seam(V, [1, 1, 2], 0.0), "must have 4 entries"),  # too short
        (Seam(H, [1, 1, 2, 3], 0.0), "must have 5 entries"),  # vertical length used by mistake
        (Seam(V, [1, 1, 2, 5], 0.0), "leaves the image"),  # column 5 in a 5-wide image
        (Seam(V, [-1, 0, 0, 0], 0.0), "leaves the image"),
        (Seam(H, [0, 1, 2, 3, 4], 0.0), "leaves the image"),  # row 4 in a 4-tall image
        (Seam(V, [1, 3, 3, 3], 0.0), "not connected"),
        (Seam(H, [0, 0, 2, 2, 2], 0.0), "not connected"),
    ],
)
def test_invalid_seams_are_rejected_with_a_clear_message(seam, message):
    with pytest.raises(ValueError, match=message):
        validate_seam(seam, (4, 5))


def test_validate_rejects_non_seams():
    with pytest.raises(TypeError):
        validate_seam(np.array([1, 1, 2, 3]), (4, 5))


def test_validate_checks_the_reported_energy_when_asked():
    validate_seam(Seam(V, [1, 1, 2, 3], 7.0), (4, 5), energy=WORKED_ENERGY)
    with pytest.raises(ValueError, match="energy along the seam is 7.0"):
        validate_seam(Seam(V, [1, 1, 2, 3], 8.0), (4, 5), energy=WORKED_ENERGY)
    with pytest.raises(ValueError, match="does not match"):
        validate_seam(Seam(V, [1, 1, 2, 3], 7.0), (4, 5), energy=WORKED_ENERGY[:3])


# --- seam_energy / seam_mask -----------------------------------------------------------


def test_seam_energy_of_the_worked_example():
    assert seam_energy(WORKED_ENERGY, Seam(V, [1, 1, 2, 3], 0.0)) == 7.0  # 1 + 2 + 1 + 3
    assert seam_energy(WORKED_ENERGY, Seam(H, [0, 1, 2, 3, 3], 0.0)) == 14.0  # 3+2+1+3+5
    assert isinstance(seam_energy(WORKED_ENERGY, Seam(V, [0, 0, 0, 0], 0.0)), float)


def test_seam_mask_marks_exactly_the_seam(rng):
    for orientation in (V, H):
        seam = random_seam(rng, (6, 9), orientation)
        mask = seam_mask((6, 9), seam)
        assert mask.dtype == bool and mask.shape == (6, 9)
        rows, cols = seam.coordinates()
        assert mask[rows, cols].all()
        assert mask.sum() == len(seam)
        per_line = mask.sum(axis=1) if orientation is V else mask.sum(axis=0)
        assert np.all(per_line == 1)  # one pixel per row (vertical) or column (horizontal)


# --- remove_seam -----------------------------------------------------------------------


def test_remove_vertical_seam_by_hand():
    image = np.arange(12, dtype=np.uint8).reshape(3, 4)
    # 0  1  2  3      remove col 1 → 0  2  3
    # 4  5  6  7      remove col 2 → 4  5  7
    # 8  9 10 11      remove col 3 → 8  9 10
    out = remove_seam(image, Seam(V, [1, 2, 3], 0.0))
    np.testing.assert_array_equal(out, [[0, 2, 3], [4, 5, 7], [8, 9, 10]])
    assert out.dtype == np.uint8


def test_remove_horizontal_seam_by_hand():
    image = np.arange(12, dtype=np.uint8).reshape(3, 4)
    # remove row 0 from col 0, row 1 from cols 1-2, row 2 from col 3
    out = remove_seam(image, Seam(H, [0, 1, 1, 2], 0.0))
    np.testing.assert_array_equal(out, [[4, 1, 2, 3], [8, 9, 10, 7]])


def _expected_after_removal(image, seam):
    """Slow, obviously-correct removal: rebuild every row/column with a Python list."""
    if seam.orientation is H:
        swapped = np.swapaxes(image, 0, 1)
        flipped = Seam(V, seam.indices, seam.energy)
        return np.swapaxes(_expected_after_removal(swapped, flipped), 0, 1)
    rows = [np.delete(image[i], seam.indices[i], axis=0) for i in range(image.shape[0])]
    return np.stack(rows)


# Every shape with both orientations, except removing a row from a 1-row image (nothing left).
REMOVAL_CASES = [
    (shape, orientation)
    for shape in [(5, 8), (8, 5), (5, 8, 3), (1, 6), (6, 2), (2, 2, 3)]
    for orientation in (V, H)
    if not (orientation is H and shape[0] == 1)
]


@pytest.mark.parametrize(("shape", "orientation"), REMOVAL_CASES)
def test_removal_matches_a_slow_reference_on_random_seams(rng, shape, orientation):
    h, w = shape[:2]
    for _ in range(20):
        image = rng.integers(0, 256, size=shape, dtype=np.uint8)
        before = image.copy()
        seam = random_seam(rng, shape, orientation)
        out = remove_seam(image, seam)
        expected_shape = (h, w - 1) if orientation is V else (h - 1, w)
        assert out.shape == expected_shape + shape[2:]
        assert out.dtype == image.dtype
        np.testing.assert_array_equal(out, _expected_after_removal(image, seam))
        np.testing.assert_array_equal(image, before)  # input untouched


def test_removal_keeps_rgb_pixels_together():
    image = np.zeros((2, 3, 3), np.uint8)
    image[:, :, 0] = [[1, 2, 3], [4, 5, 6]]  # R
    image[:, :, 1] = image[:, :, 0] * 10  # G
    image[:, :, 2] = image[:, :, 0] * 20  # B
    out = remove_seam(image, Seam(V, [0, 1], 0.0))
    assert out[:, :, 0].tolist() == [[2, 3], [4, 6]]
    assert out[0, 0].tolist() == [2, 20, 40]  # the whole pixel moved, not single channels


def test_removal_works_on_float_energy_maps_and_returns_contiguous_arrays():
    out = remove_seam(WORKED_ENERGY, Seam(H, [0, 1, 2, 3, 3], 0.0))
    assert out.shape == (3, 5) and out.dtype == np.float64
    assert out.flags["C_CONTIGUOUS"]


def test_cannot_remove_the_last_column_or_row():
    with pytest.raises(ValueError, match="1 pixel wide"):
        remove_seam(np.zeros((3, 1), np.uint8), Seam(V, [0, 0, 0], 0.0))
    with pytest.raises(ValueError, match="1 pixel tall"):
        remove_seam(np.zeros((1, 3), np.uint8), Seam(H, [0, 0, 0], 0.0))


def test_removal_rejects_a_seam_for_another_size():
    with pytest.raises(ValueError):
        remove_seam(np.zeros((4, 5), np.uint8), Seam(V, [0, 0, 0], 0.0))


def test_repeated_removal_shrinks_one_step_at_a_time(rng):
    image = rng.integers(0, 256, size=(7, 9, 3), dtype=np.uint8)
    for step in range(1, 4):
        image = remove_seam(image, random_seam(rng, image.shape, V))
        assert image.shape == (7, 9 - step, 3)
    for step in range(1, 4):
        image = remove_seam(image, random_seam(rng, image.shape, H))
        assert image.shape == (7 - step, 6, 3)


# --- highlight_seam --------------------------------------------------------------------


def test_highlight_paints_only_the_seam(rng):
    image = rng.integers(0, 200, size=(4, 5, 3), dtype=np.uint8)
    before = image.copy()
    seam = Seam(V, [1, 1, 2, 3], 0.0)
    out = highlight_seam(image, seam, colour=(255, 255, 0))
    mask = seam_mask(image.shape, seam)
    assert out.dtype == np.uint8 and out.shape == (4, 5, 3)
    assert np.all(out[mask] == [255, 255, 0])
    np.testing.assert_array_equal(out[~mask], image[~mask])
    np.testing.assert_array_equal(image, before)


def test_highlight_turns_greyscale_into_rgb():
    image = np.full((3, 2), 50, np.uint8)
    out = highlight_seam(image, Seam(H, [2, 1], 0.0))
    assert out.shape == (3, 2, 3)
    assert out[2, 0].tolist() == [255, 0, 0] and out[1, 1].tolist() == [255, 0, 0]
    assert out[0, 0].tolist() == [50, 50, 50]
