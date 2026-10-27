"""Shared pytest fixtures for the seam-carving tests (helper functions live in helpers.py)."""

from __future__ import annotations

import numpy as np
import pytest


@pytest.fixture
def rng() -> np.random.Generator:
    """Seeded generator, so every "random" test is reproducible."""
    return np.random.default_rng(3131)
