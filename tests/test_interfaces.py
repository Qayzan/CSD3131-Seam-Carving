"""Tests for the shared types in seam_carving.interfaces."""

import numpy as np
import pytest

from seam_carving.interfaces import CarveResult, Orientation, Seam, SeamFinder


def test_orientation_values():
    assert Orientation("vertical") is Orientation.VERTICAL
    assert Orientation("horizontal") is Orientation.HORIZONTAL
    assert len(Orientation) == 2


@pytest.mark.parametrize("dtype", [np.int8, np.int32, np.int64, np.intp, np.uint16])
def test_seam_normalises_any_integer_dtype_to_int64(dtype):
    seam = Seam(Orientation.VERTICAL, np.array([0, 1, 1], dtype=dtype), 5)
    assert seam.indices.dtype == np.int64
    assert seam.indices.tolist() == [0, 1, 1]
    assert isinstance(seam.energy, float) and seam.energy == 5.0


def test_seam_accepts_a_plain_list():
    assert Seam(Orientation.HORIZONTAL, [2, 1, 0], 0.0).indices.tolist() == [2, 1, 0]


def test_seam_copies_and_freezes_its_indices():
    source = np.array([0, 1, 2])
    seam = Seam(Orientation.VERTICAL, source, 1.0)
    source[0] = 99  # changing the caller's array must not change the seam
    assert seam.indices[0] == 0
    with pytest.raises(ValueError):
        seam.indices[0] = 5  # read-only
    with pytest.raises(AttributeError):
        seam.energy = 2.0  # frozen dataclass


@pytest.mark.parametrize(
    ("indices", "error"),
    [
        (np.array([0.0, 1.0]), TypeError),  # floats
        (np.array([True, False]), TypeError),  # booleans
        (np.array([[0, 1]]), ValueError),  # 2-D
        (np.array([], dtype=np.int64), ValueError),  # empty
    ],
)
def test_seam_rejects_malformed_indices(indices, error):
    with pytest.raises(error):
        Seam(Orientation.VERTICAL, indices, 0.0)


def test_seam_rejects_a_string_orientation():
    with pytest.raises(TypeError):
        Seam("vertical", np.array([0]), 0.0)


def test_seam_equality_compares_values():
    a = Seam(Orientation.VERTICAL, np.array([1, 1, 2]), 7.0)
    assert a == Seam(Orientation.VERTICAL, [1, 1, 2], 7)
    assert a != Seam(Orientation.VERTICAL, [1, 2, 2], 7.0)
    assert a != Seam(Orientation.HORIZONTAL, [1, 1, 2], 7.0)
    assert a != Seam(Orientation.VERTICAL, [1, 1, 2], 8.0)
    assert a != "not a seam"
    with pytest.raises(TypeError):
        hash(a)


def test_seam_length_and_coordinates():
    vertical = Seam(Orientation.VERTICAL, np.array([1, 1, 2, 3]), 7.0)
    rows, cols = vertical.coordinates()
    assert len(vertical) == 4
    assert rows.tolist() == [0, 1, 2, 3] and cols.tolist() == [1, 1, 2, 3]

    horizontal = Seam(Orientation.HORIZONTAL, np.array([2, 1, 1]), 0.0)
    rows, cols = horizontal.coordinates()
    assert rows.tolist() == [2, 1, 1] and cols.tolist() == [0, 1, 2]


def test_any_matching_callable_is_a_seam_finder():
    def leftmost(energy, orientation):
        length = energy.shape[0] if orientation is Orientation.VERTICAL else energy.shape[1]
        return Seam(orientation, np.zeros(length, dtype=np.int64), 0.0)

    assert isinstance(leftmost, SeamFinder)
    assert not isinstance(42, SeamFinder)


def test_carve_result_defaults_are_independent():
    a = CarveResult(image=np.zeros((2, 2), np.uint8))
    b = CarveResult(image=np.zeros((2, 2), np.uint8))
    a.seams.append(Seam(Orientation.VERTICAL, [0, 0], 0.0))
    a.timings["total"] = 1.0
    assert b.seams == [] and b.timings == {} and b.frames is None
