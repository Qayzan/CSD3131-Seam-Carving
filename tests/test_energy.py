"""Tests for seam_carving.energy: the OpenCV Sobel energy and its NumPy cross-check."""

import numpy as np
import pytest

from seam_carving.energy import gradient_energy, gradient_energy_reference

SHAPES = [(1, 1), (1, 6), (6, 1), (2, 2), (4, 5), (5, 4), (17, 23)]


@pytest.mark.parametrize("shape", SHAPES)
@pytest.mark.parametrize("colour", [False, True])
def test_opencv_and_numpy_reference_agree(rng, shape, colour):
    """Independent check: a wrong kernel size, border mode or dtype would break this."""
    full_shape = shape + (3,) if colour else shape
    image = rng.integers(0, 256, size=full_shape, dtype=np.uint8)
    np.testing.assert_allclose(
        gradient_energy(image), gradient_energy_reference(image), rtol=0, atol=1e-9
    )


@pytest.mark.parametrize("shape", SHAPES)
def test_output_is_float64_hw_and_non_negative(rng, shape):
    image = rng.integers(0, 256, size=shape + (3,), dtype=np.uint8)
    energy = gradient_energy(image)
    assert energy.dtype == np.float64
    assert energy.shape == shape
    assert np.all(energy >= 0)


def test_flat_image_has_zero_energy_everywhere_including_borders():
    """With zero padding the border pixels would look like edges; reflection avoids that."""
    image = np.full((5, 6, 3), 200, np.uint8)
    assert np.all(gradient_energy(image) == 0.0)


def test_vertical_edge_by_hand():
    # Columns 0 0 | 10 10. At column 1 the Sobel x-kernel sees (10 - 0) weighted 1 + 2 + 1,
    # so |dI/dx| = 40; the same at column 2. Columns 0 and 3 mirror their neighbour (reflect
    # border), so their left and right neighbours are equal and the derivative is 0.
    # No change from top to bottom, so |dI/dy| = 0 everywhere.
    image = np.array([[0, 0, 10, 10]] * 3, dtype=np.float64)
    expected = np.array([[0, 40, 40, 0]] * 3, dtype=np.float64)
    np.testing.assert_array_equal(gradient_energy(image), expected)


def test_horizontal_edge_is_the_transposed_case():
    image = np.array([[0, 0, 10, 10]] * 3, dtype=np.float64)
    np.testing.assert_array_equal(gradient_energy(image.T), gradient_energy(image).T)


def test_uint8_input_does_not_wrap_around():
    """200 → 250 and 250 → 200 are equally strong edges; uint8 arithmetic would break this."""
    up = np.array([[200, 200, 250, 250]] * 3, dtype=np.uint8)
    down = np.array([[250, 250, 200, 200]] * 3, dtype=np.uint8)
    np.testing.assert_array_equal(gradient_energy(up), gradient_energy(down))
    assert gradient_energy(up).max() == 4 * 50


def test_colour_energy_uses_luminance():
    image = np.zeros((3, 4, 3), np.uint8)
    image[:, 2:, 1] = 100  # green step: luminance step of 0.587 * 100
    np.testing.assert_allclose(gradient_energy(image)[1], [0, 4 * 58.7, 4 * 58.7, 0])


def test_input_is_not_modified(rng):
    image = rng.integers(0, 256, size=(4, 5, 3), dtype=np.uint8)
    before = image.copy()
    gradient_energy(image)
    gradient_energy_reference(image)
    np.testing.assert_array_equal(image, before)


def test_invalid_images_are_rejected():
    with pytest.raises(ValueError):
        gradient_energy(np.zeros((3, 3, 4), np.uint8))
    with pytest.raises(TypeError):
        gradient_energy(np.zeros((3, 3), np.int32))
