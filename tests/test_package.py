"""Milestone 1 smoke tests: the package installs, imports and declares every module.

Real algorithm tests (test_energy.py, test_dynamic_programming.py, test_integration.py, …)
are added with the implementations in later milestones.
"""

import importlib

import pytest

import seam_carving

MODULES = [
    "image_utils",
    "energy",
    "interfaces",
    "seam_ops",
    "carver",
    "dynamic_programming",
    "greedy",
    "graph_cut",
]


def test_version_is_declared():
    assert seam_carving.__version__ == "0.1.0"


@pytest.mark.parametrize("name", MODULES)
def test_module_imports_and_is_documented(name):
    module = importlib.import_module(f"seam_carving.{name}")
    assert module.__doc__, f"seam_carving.{name} needs a module docstring"
    assert "Owner:" in module.__doc__, f"seam_carving.{name} must name its owner"


def test_third_party_dependencies_available():
    cv2 = pytest.importorskip("cv2")
    np = pytest.importorskip("numpy")
    assert hasattr(cv2, "Sobel")
    assert np.zeros((2, 2)).shape == (2, 2)
