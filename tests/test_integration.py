"""End-to-end tests: real energy + DP finder + carving loop, checked against independent
oracles and against what seam carving is supposed to achieve."""

import numpy as np
import pytest

from helpers import brute_force_find_seam
from seam_carving.carver import carve
from seam_carving.dynamic_programming import find_seam
from seam_carving.energy import gradient_energy
from seam_carving.image_utils import load_image, save_image


@pytest.mark.parametrize("seed", range(12))
def test_dp_and_exhaustive_search_carve_identical_images(seed):
    """Greyscale uint8 images give integer energies, so both finders must agree exactly,
    seam by seam, through the whole repeated-resizing loop."""
    rng = np.random.default_rng(seed)
    image = rng.integers(0, 256, size=(5, 6), dtype=np.uint8)
    dp = carve(image, 3, 3, finder=find_seam, order="alternate")
    oracle = carve(image, 3, 3, finder=brute_force_find_seam, order="alternate")
    assert dp.seams == oracle.seams
    np.testing.assert_array_equal(dp.image, oracle.image)


def _scene():
    """Flat sky and ground with a high-contrast 'object' in the middle."""
    image = np.empty((40, 60, 3), np.uint8)
    image[:25] = (135, 190, 235)  # sky
    image[25:] = (90, 160, 70)  # grass
    image[14:32, 26:36] = (200, 30, 30)  # red object, 18 x 10 pixels
    image[18:22, 29:33] = (250, 240, 60)  # yellow detail inside it
    return image


def _count(image, colour):
    return int(np.all(image == colour, axis=-1).sum())


def test_content_aware_width_reduction_keeps_the_object():
    image = _scene()
    result = carve(image, 40, 42)  # remove 30% of the width
    # Every seam went through the flat background; the object is untouched.
    assert _count(result.image, (200, 30, 30)) == _count(image, (200, 30, 30))
    assert _count(result.image, (250, 240, 60)) == _count(image, (250, 240, 60))
    # Plain resizing would have squashed it instead (for contrast, not a requirement).


def test_content_aware_height_reduction_keeps_the_object():
    image = _scene()
    result = carve(image, 30, 60)
    assert _count(result.image, (250, 240, 60)) == _count(image, (250, 240, 60))


def test_every_seam_is_minimal_for_the_energy_at_that_step():
    """Replay the carve and confirm each seam's cost equals the DP optimum at that moment."""
    image = _scene()[::4, ::4]  # 10 x 15, small enough to check every step
    result = carve(image, 8, 11, keep_frames=True)
    for frame, seam in zip(result.frames[:-1], result.seams, strict=True):
        energy = gradient_energy(frame)
        assert seam.energy == pytest.approx(find_seam(energy, seam.orientation).energy)


def test_file_round_trip(tmp_path, rng):
    image = rng.integers(0, 256, size=(20, 24, 3), dtype=np.uint8)
    source = save_image(tmp_path / "in.png", image)
    result = carve(load_image(source), 16, 18)
    out = save_image(tmp_path / "out.png", result.image)
    np.testing.assert_array_equal(load_image(out), result.image)
