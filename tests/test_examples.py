"""Smoke tests: the demo scripts in examples/ run end to end and write their outputs."""

import importlib.util
from pathlib import Path

import numpy as np

from seam_carving.image_utils import load_image, save_image

EXAMPLES = Path(__file__).resolve().parents[1] / "examples"


def _load(name):
    spec = importlib.util.spec_from_file_location(name, EXAMPLES / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_sample_image_is_deterministic():
    make = _load("make_sample_image").make_scene
    first, second = make(), make()
    assert first.shape == (300, 480, 3) and first.dtype == np.uint8
    np.testing.assert_array_equal(first, second)


def test_committed_sample_image_matches_the_generator():
    committed = load_image(EXAMPLES.parent / "assets" / "images" / "synthetic_scene.png")
    np.testing.assert_array_equal(committed, _load("make_sample_image").make_scene())


def test_demo_writes_every_output(tmp_path, rng):
    source = save_image(tmp_path / "tiny.png", rng.integers(0, 256, (24, 32, 3), np.uint8))
    out = tmp_path / "out"
    demo = _load("demo_dp")
    assert (
        demo.main([str(source), "--width", "0.75", "--height", "20", "--out", str(out), "--frames"])
        == 0
    )
    names = sorted(p.name for p in out.glob("*.png"))
    assert names == [
        "01_original.png",
        "02_energy.png",
        "03_cumulative_energy.png",
        "04_first_vertical_seam.png",
        "05_first_horizontal_seam.png",
        "06_all_removed_seams.png",
        "07_carved.png",
        "08_comparison.png",
    ]
    assert load_image(out / "07_carved.png").shape == (20, 24, 3)
    assert len(list((out / "frames").glob("*.png"))) == (32 - 24) + (24 - 20) + 1


def test_target_size_parsing():
    target_size = _load("demo_dp").target_size
    assert target_size("0.7", 480) == 336  # a fraction of the current size
    assert target_size("320", 480) == 320  # an absolute number of pixels
    assert target_size("999", 480) == 480  # never larger than the image
    assert target_size("0.0001", 480) == 1  # never smaller than 1
