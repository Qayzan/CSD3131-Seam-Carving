"""Question 1 demo: seam carving with Dynamic Programming, step by step.

Saves every picture the report and the presentation need into ``examples/output/<name>/``
(git-ignored) and prints the timings:

    01_original.png               the input
    02_energy.png                 energy map |dI/dx| + |dI/dy| (Sobel), brighter = more important
    03_cumulative_energy.png      DP table M for vertical seams (bottom rows = whole seams)
    04_first_vertical_seam.png    cheapest vertical seam (red)
    05_first_horizontal_seam.png  cheapest horizontal seam (blue)
    06_all_removed_seams.png      every removed seam drawn on the ORIGINAL picture
    07_carved.png                 the result
    08_comparison.png             original | plain resize | centre crop | seam carving
    frames/frame_0000.png …       (with --frames) each step with the next seam highlighted

Run from the repository root (after ``pip install -e ".[dev]"``):

    python examples/demo_dp.py                                   # synthetic scene, width −30%
    python examples/demo_dp.py path/to/photo.jpg --width 0.7 --height 0.9
    python examples/demo_dp.py photo.jpg --width 320 --height 240 --order alternate --frames
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import cv2
import numpy as np

from seam_carving.carver import ORDERS, carve, seams_in_original_coordinates
from seam_carving.dynamic_programming import cumulative_energy, find_seam
from seam_carving.energy import gradient_energy
from seam_carving.image_utils import load_image, save_image
from seam_carving.interfaces import Orientation
from seam_carving.seam_ops import highlight_seam

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_IMAGE = ROOT / "assets" / "images" / "synthetic_scene.png"
RED, BLUE = (230, 30, 30), (30, 90, 230)


def shown(path: Path) -> str:
    """Path relative to the repository when possible (shorter console output)."""
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def target_size(value: str, current: int) -> int:
    """``0.7`` means 70% of the current size; ``320`` means 320 pixels."""
    number = float(value)
    size = round(current * number) if number <= 1.0 else int(number)
    return max(1, min(current, size))


def to_heatmap(values: np.ndarray) -> np.ndarray:
    """Scale any 2-D array to 0..255 and colour it (dark = low, bright yellow = high), RGB."""
    lo, hi = float(values.min()), float(values.max())
    scaled = np.zeros(values.shape, np.uint8) if hi == lo else ((values - lo) / (hi - lo) * 255)
    bgr = cv2.applyColorMap(np.asarray(scaled, np.uint8), cv2.COLORMAP_INFERNO)
    return cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)


def labelled(image: np.ndarray, text: str, height: int) -> np.ndarray:
    """Pad ``image`` to ``height`` rows and put a caption strip on top."""
    rgb = image if image.ndim == 3 else np.repeat(image[:, :, None], 3, axis=2)
    pad = np.full((height - rgb.shape[0], rgb.shape[1], 3), 255, np.uint8)
    caption = np.full((26, max(rgb.shape[1], 8 * len(text) + 10), 3), 255, np.uint8)
    cv2.putText(caption, text, (5, 18), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1, cv2.LINE_AA)
    body = np.vstack([rgb, pad])
    if caption.shape[1] > body.shape[1]:
        body = np.hstack(
            [body, np.full((height, caption.shape[1] - body.shape[1], 3), 255, np.uint8)]
        )
    return np.vstack([caption, body])


def side_by_side(panels: list[tuple[str, np.ndarray]]) -> np.ndarray:
    height = max(p.shape[0] for _, p in panels)
    gap = np.full((height + 26, 12, 3), 255, np.uint8)
    parts = []
    for name, panel in panels:
        parts += [labelled(panel, name, height), gap]
    return np.hstack(parts[:-1])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("image", nargs="?", default=str(DEFAULT_IMAGE), help="input picture")
    parser.add_argument("--width", default="0.7", help="target width: fraction (0.7) or pixels")
    parser.add_argument("--height", default="1.0", help="target height: fraction or pixels")
    parser.add_argument("--order", choices=ORDERS, default="width_first")
    parser.add_argument(
        "--out", default=None, help="output folder (default examples/output/<name>)"
    )
    parser.add_argument("--frames", action="store_true", help="also save every step as a PNG")
    args = parser.parse_args(argv)

    source = Path(args.image)
    if source == DEFAULT_IMAGE and not source.exists():
        from make_sample_image import main as make_sample

        make_sample()
    image = load_image(source)
    h, w = image.shape[:2]
    th, tw = target_size(args.height, h), target_size(args.width, w)
    out = Path(args.out) if args.out else ROOT / "examples" / "output" / source.stem
    out.mkdir(parents=True, exist_ok=True)
    print(f"input  {shown(source)}  {h} x {w}  →  target {th} x {tw}  ({args.order})")

    # The first step, shown in detail (this is what Q1(a) and Q1(b) ask for).
    energy = gradient_energy(image)
    table, _ = cumulative_energy(energy)
    first_v = find_seam(energy, Orientation.VERTICAL)
    first_h = find_seam(energy, Orientation.HORIZONTAL)
    save_image(out / "01_original.png", image)
    save_image(out / "02_energy.png", to_heatmap(energy))
    save_image(out / "03_cumulative_energy.png", to_heatmap(table))
    save_image(out / "04_first_vertical_seam.png", highlight_seam(image, first_v, RED))
    save_image(out / "05_first_horizontal_seam.png", highlight_seam(image, first_h, BLUE))
    print(f"first vertical seam energy {first_v.energy:.1f}, horizontal {first_h.energy:.1f}")

    # The full repeated resizing (Q1(c)): energy recomputed before every seam.
    result = carve(image, th, tw, find_seam, order=args.order, keep_frames=args.frames)
    overlay = image.copy() if image.ndim == 3 else np.repeat(image[:, :, None], 3, axis=2)
    for seam, (rows, cols) in zip(
        result.seams, seams_in_original_coordinates(image.shape, result.seams), strict=True
    ):
        overlay[rows, cols] = RED if seam.orientation is Orientation.VERTICAL else BLUE
    save_image(out / "06_all_removed_seams.png", overlay)
    save_image(out / "07_carved.png", result.image)

    # Comparison with the two naive ways of changing the size.
    scaled = cv2.resize(image, (tw, th), interpolation=cv2.INTER_AREA)
    top, left = (h - th) // 2, (w - tw) // 2
    cropped = image[top : top + th, left : left + tw]
    comparison = side_by_side(
        [
            ("original", image),
            ("plain resize", scaled),
            ("centre crop", cropped),
            ("seam carving (DP)", result.image),
        ]
    )
    save_image(out / "08_comparison.png", comparison)

    if args.frames:
        frames_dir = out / "frames"
        for k, (frame, seam) in enumerate(zip(result.frames, result.seams + [None], strict=True)):
            picture = frame if seam is None else highlight_seam(frame, seam, RED)
            save_image(frames_dir / f"frame_{k:04d}.png", picture)

    t = result.timings
    n = max(1, len(result.seams))
    print(f"removed {len(result.seams)} seams in {t['total']:.3f} s")
    print(
        f"  energy {t['energy']:.3f} s | DP find_seam {t['find_seam']:.3f} s "
        f"({1000 * t['find_seam'] / n:.2f} ms/seam) | removal {t['remove']:.3f} s"
    )
    print(f"outputs in {shown(out)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
