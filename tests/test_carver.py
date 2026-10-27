"""Tests for the shared carving loop (seam_carving.carver)."""

import numpy as np
import pytest

from seam_carving.carver import ORDERS, carve, removal_plan, seams_in_original_coordinates
from seam_carving.energy import gradient_energy
from seam_carving.interfaces import CarveResult, Orientation, Seam

V, H = Orientation.VERTICAL, Orientation.HORIZONTAL


def first_line_finder(energy, orientation):
    """A dummy finder that always removes column 0 (vertical) or row 0 (horizontal)."""
    length = energy.shape[0] if orientation is V else energy.shape[1]
    idx = np.zeros(length, dtype=np.int64)
    rows, cols = (np.arange(length), idx) if orientation is V else (idx, np.arange(length))
    return Seam(orientation, idx, float(energy[rows, cols].sum()))


@pytest.fixture
def image(rng):
    return rng.integers(0, 256, size=(12, 15, 3), dtype=np.uint8)


# --- target sizes ----------------------------------------------------------------------------


@pytest.mark.parametrize("order", ORDERS)
@pytest.mark.parametrize("target", [(12, 15), (12, 10), (7, 15), (5, 6), (1, 1)])
def test_reaches_exactly_the_target_size(image, order, target):
    result = carve(image, *target, order=order)
    assert isinstance(result, CarveResult)
    assert result.image.shape == target + (3,)
    assert result.image.dtype == np.uint8
    assert len(result.seams) == (12 - target[0]) + (15 - target[1])


def test_greyscale_images_work(rng):
    grey = rng.integers(0, 256, size=(9, 11), dtype=np.uint8)
    assert carve(grey, 6, 8).image.shape == (6, 8)


def test_no_change_returns_a_copy(image):
    result = carve(image, 12, 15)
    assert result.seams == [] and result.image is not image
    np.testing.assert_array_equal(result.image, image)


def test_input_image_is_not_modified(image):
    before = image.copy()
    carve(image, 8, 9)
    np.testing.assert_array_equal(image, before)


@pytest.mark.parametrize(
    ("target", "error"),
    [
        ((13, 15), ValueError),  # taller than the image: enlargement is out of scope
        ((12, 16), ValueError),
        ((0, 5), ValueError),
        ((5, -1), ValueError),
        ((5.0, 5), TypeError),
        ((True, 5), TypeError),
    ],
)
def test_invalid_targets_are_rejected(image, target, error):
    with pytest.raises(error):
        carve(image, *target)


def test_unknown_order_is_rejected(image):
    with pytest.raises(ValueError):
        carve(image, 10, 10, order="random")


def test_numpy_integer_targets_are_accepted(image):
    assert carve(image, np.int64(10), np.int32(10)).image.shape == (10, 10, 3)


# --- order of removals -----------------------------------------------------------------------


def test_removal_plans():
    assert list(removal_plan(2, 1, "width_first")) == [V, V, H]
    assert list(removal_plan(2, 1, "height_first")) == [H, V, V]
    assert list(removal_plan(3, 1, "alternate")) == [V, H, V, V]
    assert list(removal_plan(0, 0, "alternate")) == []


@pytest.mark.parametrize("order", ORDERS)
def test_seams_follow_the_requested_order(image, order):
    result = carve(image, 10, 12, order=order)
    assert [s.orientation for s in result.seams] == list(removal_plan(3, 2, order))


# --- the energy is recomputed before every seam (brief requirement R1c) ----------------------


def test_energy_is_recomputed_for_every_seam_on_the_current_image(image):
    shapes = []

    def spy_energy(current):
        shapes.append(current.shape[:2])
        return gradient_energy(current)

    carve(image, 10, 12, energy_fn=spy_energy)
    # 3 vertical seams (width 15 → 12), then 2 horizontal (height 12 → 10), each on the
    # image as it is at that moment.
    assert shapes == [(12, 15), (12, 14), (12, 13), (12, 12), (11, 12)]


# --- the finder is plugged in, checked and timed ---------------------------------------------


def test_any_finder_can_be_plugged_in(image):
    result = carve(image, 9, 11, finder=first_line_finder)
    # Removing column 0 four times and then row 0 three times is a crop of the top-left.
    np.testing.assert_array_equal(result.image, image[3:, 4:])


def test_invalid_seams_from_a_finder_are_caught(image):
    def broken(energy, orientation):
        return Seam(orientation, np.full(energy.shape[0], 99), 0.0)  # out of range

    with pytest.raises(ValueError, match="leaves the image"):
        carve(image, 12, 14, finder=broken)


def test_wrong_reported_energy_is_caught(image):
    def liar(energy, orientation):
        seam = first_line_finder(energy, orientation)
        return Seam(orientation, seam.indices, seam.energy + 1.0)

    with pytest.raises(ValueError, match="energy along the seam"):
        carve(image, 12, 14, finder=liar)


def test_wrong_orientation_is_caught(image):
    with pytest.raises(ValueError, match="asked for vertical"):
        carve(image, 12, 14, finder=lambda e, o: first_line_finder(e, H))


def test_finders_cannot_modify_the_energy_map(image):
    def vandal(energy, orientation):
        energy[0, 0] = 0.0  # contract rule 1 forbids this
        return first_line_finder(energy, orientation)

    with pytest.raises(ValueError):
        carve(image, 12, 14, finder=vandal)


def test_timings_are_reported(image):
    timings = carve(image, 10, 12).timings
    assert set(timings) == {"energy", "find_seam", "remove", "total"}
    assert all(t >= 0 for t in timings.values())
    assert timings["total"] >= timings["energy"] + timings["find_seam"] + timings["remove"]


def test_frames_show_every_step(image):
    result = carve(image, 10, 13, keep_frames=True)
    assert len(result.frames) == len(result.seams) + 1 == 5
    np.testing.assert_array_equal(result.frames[0], image)
    np.testing.assert_array_equal(result.frames[-1], result.image)
    assert [f.shape[:2] for f in result.frames] == [
        (12, 15),
        (12, 14),
        (12, 13),
        (11, 13),
        (10, 13),
    ]
    assert carve(image, 10, 13).frames is None


# --- mapping seams back to the original picture ----------------------------------------------


def test_original_coordinates_with_the_crop_finder(image):
    result = carve(image, 10, 12, finder=first_line_finder)
    positions = seams_in_original_coordinates(image.shape, result.seams)
    # The first three seams were original columns 0, 1, 2 (each over all 12 rows) …
    for k in range(3):
        rows, cols = positions[k]
        assert rows.tolist() == list(range(12)) and cols.tolist() == [k] * 12
    # … then rows 0 and 1, but only over the columns that were still there (3..14).
    for k in range(2):
        rows, cols = positions[3 + k]
        assert rows.tolist() == [k] * 12 and cols.tolist() == list(range(3, 15))


@pytest.mark.parametrize("order", ORDERS)
def test_original_coordinates_cover_exactly_the_removed_pixels(image, order):
    result = carve(image, 8, 10, order=order)
    positions = seams_in_original_coordinates(image.shape, result.seams)
    removed = np.zeros(image.shape[:2], dtype=int)
    for rows, cols in positions:
        removed[rows, cols] += 1
    assert removed.max() == 1  # no pixel removed twice
    assert removed.sum() == image.shape[0] * image.shape[1] - 8 * 10


@pytest.mark.parametrize(("target", "axis"), [((12, 9), 1), ((7, 15), 0)])
def test_surviving_pixels_are_the_carved_image(image, target, axis):
    """With one orientation only, the pixels NOT covered by any mapped seam, read row by row
    (vertical seams) or column by column (horizontal seams), must be the carved image."""
    result = carve(image, *target)
    removed = np.zeros(image.shape[:2], dtype=bool)
    for rows, cols in seams_in_original_coordinates(image.shape, result.seams):
        removed[rows, cols] = True
    if axis == 1:
        kept = image[~removed].reshape(target + (3,))
    else:
        kept = image.transpose(1, 0, 2)[~removed.T].reshape((target[1], target[0], 3))
        kept = kept.transpose(1, 0, 2)
    np.testing.assert_array_equal(kept, result.image)
