# Examples

Runnable demonstrations. Generated files go to `examples/output/`, which is git-ignored, so re-run the scripts to recreate them.

## `demo_dp.py` — Question 1 (Dynamic Programming), step by step

```bash
python examples/demo_dp.py                                   # synthetic scene, width −30%
python examples/demo_dp.py path/to/photo.jpg --width 0.7 --height 0.9
python examples/demo_dp.py photo.jpg --width 320 --height 240 --order alternate --frames
```

`--width` / `--height` take a fraction of the current size (`0.7`) or a number of pixels (`320`). It writes `examples/output/<image name>/`:

| File | Shows |
|---|---|
| `01_original.png` | the input |
| `02_energy.png` | energy map (Sobel), brighter = more important — Q1(a) |
| `03_cumulative_energy.png` | the DP table M for vertical seams — Q1(b) |
| `04_first_vertical_seam.png`, `05_first_horizontal_seam.png` | the cheapest seams (red / blue) — Q1(b) |
| `06_all_removed_seams.png` | every removed seam drawn on the original picture — bonus 7a |
| `07_carved.png` | the result after repeated removal with the energy recomputed each time — Q1(c) |
| `08_comparison.png` | original, plain resize, centre crop and seam carving side by side |
| `frames/frame_0000.png …` | with `--frames`: every step with the next seam highlighted (for the video) |

It also prints the timings (energy, DP, removal; milliseconds per seam).

## `make_sample_image.py`

Draws `assets/images/synthetic_scene.png`, a picture made entirely by our code (flat sky and grass, plus a house, tree, sun and mountains with strong edges), so it is free to redistribute.
