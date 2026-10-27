"""Tests for seam_carving.image_utils (conventions, colour order, file I/O)."""

import cv2
import numpy as np
import pytest

from seam_carving.image_utils import (
    LUMA_WEIGHTS,
    load_image,
    save_image,
    to_grayscale,
    validate_image,
)

# --- validate_image --------------------------------------------------------------------


@pytest.mark.parametrize(
    "image",
    [
        np.zeros((1, 1), np.uint8),
        np.zeros((4, 7, 3), np.uint8),
        np.zeros((3, 2), np.float64),
        np.zeros((3, 2, 3), np.float32),
    ],
)
def test_validate_image_accepts_the_convention(image):
    assert validate_image(image) is image


@pytest.mark.parametrize(
    ("image", "error"),
    [
        ([[1, 2], [3, 4]], TypeError),  # a list, not an array
        (np.zeros((4,), np.uint8), ValueError),  # 1-D
        (np.zeros((2, 2, 4), np.uint8), ValueError),  # RGBA
        (np.zeros((2, 2, 1, 1), np.uint8), ValueError),  # 4-D
        (np.zeros((0, 3), np.uint8), ValueError),  # empty
        (np.zeros((2, 2), bool), TypeError),
        (np.zeros((2, 2), np.int32), TypeError),
        (np.zeros((2, 2), np.uint16), TypeError),
    ],
)
def test_validate_image_rejects_everything_else(image, error):
    with pytest.raises(error):
        validate_image(image)


# --- to_grayscale ----------------------------------------------------------------------


def test_grayscale_uses_bt601_weights_without_rounding():
    image = np.array([[[255, 0, 0], [0, 255, 0], [0, 0, 255], [10, 20, 30]]], np.uint8)
    gray = to_grayscale(image)
    assert gray.dtype == np.float64 and gray.shape == (1, 4)
    expected = [0.299 * 255, 0.587 * 255, 0.114 * 255, 0.299 * 10 + 0.587 * 20 + 0.114 * 30]
    np.testing.assert_allclose(gray[0], expected)
    assert LUMA_WEIGHTS.sum() == pytest.approx(1.0)


def test_grayscale_matches_opencv_up_to_its_rounding(rng):
    image = rng.integers(0, 256, size=(9, 13, 3), dtype=np.uint8)
    ours = to_grayscale(image)
    theirs = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY).astype(np.float64)
    assert np.max(np.abs(ours - theirs)) <= 0.5 + 1e-9


def test_grayscale_of_a_grey_image_is_unchanged_and_a_copy():
    image = np.array([[0, 128, 255]], np.uint8)
    gray = to_grayscale(image)
    assert gray.dtype == np.float64 and gray.tolist() == [[0.0, 128.0, 255.0]]
    gray[0, 0] = 99
    assert image[0, 0] == 0


def test_grayscale_works_in_float_so_differences_do_not_wrap():
    gray = to_grayscale(np.array([[200, 250]], np.uint8))
    assert gray[0, 0] - gray[0, 1] == -50.0  # in uint8 this would be 206


# --- load_image / save_image -----------------------------------------------------------


def test_png_round_trip_keeps_rgb_order(tmp_path, rng):
    image = rng.integers(0, 256, size=(5, 7, 3), dtype=np.uint8)
    image[0, 0] = (255, 0, 0)  # pure red must stay red, not turn blue
    path = save_image(tmp_path / "rgb.png", image)
    loaded = load_image(path)
    assert loaded.dtype == np.uint8 and loaded.shape == (5, 7, 3)
    np.testing.assert_array_equal(loaded, image)
    assert loaded[0, 0].tolist() == [255, 0, 0]


def test_files_are_stored_as_bgr_for_other_programs(tmp_path):
    red = np.zeros((2, 2, 3), np.uint8)
    red[..., 0] = 255
    path = save_image(tmp_path / "red.png", red)
    raw = cv2.imdecode(np.fromfile(path, np.uint8), cv2.IMREAD_COLOR)  # OpenCV = BGR
    assert raw[0, 0].tolist() == [0, 0, 255]


def test_greyscale_round_trip_stays_2d(tmp_path, rng):
    image = rng.integers(0, 256, size=(6, 3), dtype=np.uint8)
    loaded = load_image(save_image(tmp_path / "grey.png", image))
    assert loaded.shape == (6, 3)
    np.testing.assert_array_equal(loaded, image)


def test_alpha_channel_is_dropped(tmp_path):
    bgra = np.zeros((3, 4, 4), np.uint8)
    bgra[..., 0] = 255  # blue in OpenCV's order
    bgra[..., 3] = 128  # half transparent
    path = tmp_path / "alpha.png"
    cv2.imencode(".png", bgra)[1].tofile(path)
    loaded = load_image(path)
    assert loaded.shape == (3, 4, 3)
    assert loaded[0, 0].tolist() == [0, 0, 255]  # blue, now in RGB order


def test_16_bit_images_are_scaled_to_8_bit(tmp_path):
    deep = np.array([[0, 257, 65535]], np.uint16)
    path = tmp_path / "deep.png"
    cv2.imencode(".png", deep)[1].tofile(path)
    loaded = load_image(path)
    assert loaded.dtype == np.uint8 and loaded.tolist() == [[0, 1, 255]]


def test_non_ascii_paths_work(tmp_path):
    image = np.full((2, 3, 3), 77, np.uint8)
    path = save_image(tmp_path / "ảnh 圖像" / "dữ liệu.png", image)  # also creates the folder
    np.testing.assert_array_equal(load_image(path), image)


def test_missing_file_raises_file_not_found(tmp_path):
    with pytest.raises(FileNotFoundError):
        load_image(tmp_path / "nope.png")


@pytest.mark.parametrize("content", [b"", b"this is not an image"])
def test_undecodable_file_raises_value_error(tmp_path, content):
    path = tmp_path / "broken.png"
    path.write_bytes(content)
    with pytest.raises(ValueError):
        load_image(path)


def test_save_rejects_float_images(tmp_path):
    with pytest.raises(TypeError):
        save_image(tmp_path / "x.png", np.zeros((2, 2), np.float64))


@pytest.mark.parametrize("name", ["no_extension", "weird.notaformat"])
def test_save_rejects_unknown_formats(tmp_path, name):
    with pytest.raises(ValueError):
        save_image(tmp_path / name, np.zeros((2, 2), np.uint8))
