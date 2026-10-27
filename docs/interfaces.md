# Shared interface contract (DRAFT — finalised in Milestone 2)

This is the agreement that lets FF, HM and WN build their seam finders independently and still plug them into the same carver, tests, benchmarks and demo. **Please review and comment before Milestone 2 code is merged.** After that, changes need a pull request reviewed by FF.

## 1. Arrays

| Thing | Type | Shape | Notes |
|---|---|---|---|
| Colour image | `np.ndarray`, `uint8` | `(H, W, 3)` | **RGB** order (converted from OpenCV's BGR by `image_utils.load_image`) |
| Greyscale image | `np.ndarray`, `uint8` | `(H, W)` | accepted everywhere a colour image is |
| Energy map | `np.ndarray`, `float64` | `(H, W)` | non-negative; produced by `energy.gradient_energy` |

Indexing is always `array[row, col]`. Row 0 is the top row and column 0 is the left column.

## 2. Types (to be implemented in `seam_carving/interfaces.py`)

```python
class Orientation(enum.Enum):
    VERTICAL = "vertical"  # top → bottom; removes one pixel from every ROW (width − 1)
    HORIZONTAL = "horizontal"  # left → right; removes one pixel from every COLUMN (height − 1)


@dataclass(frozen=True)
class Seam:
    orientation: Orientation
    indices: np.ndarray  # int64
    # VERTICAL:   len == H, indices[i] = column removed in row i
    # HORIZONTAL: len == W, indices[j] = row removed in column j
    energy: float  # sum of energy along the seam (same energy map the finder received)


class SeamFinder(Protocol):
    def __call__(self, energy: np.ndarray, orientation: Orientation) -> Seam: ...
```

Rules every `SeamFinder` must follow:

1. It must **not modify** `energy` (the shared carver reuses it for visualisation).
2. The returned seam must be **valid**: correct length; indices in range; consecutive indices differ by at most 1. `seam_ops.validate_seam` checks this, and the carver calls it after every finder call.
3. `Seam.energy` must equal `seam_ops.seam_energy(energy, seam)` (within floating-point tolerance).
4. Results must be deterministic for the same input.
5. It must not use any library content-aware resizing / seam-carving function (brief rule).

Each algorithm module exposes one public entry point with this signature:

```python
# dynamic_programming.py (FF), greedy.py (HM), graph_cut.py (WN)
def find_seam(energy: np.ndarray, orientation: Orientation) -> Seam: ...
```

Algorithm-specific extras (the DP table M, a residual graph, …) go in optional helper functions or a `debug=True` variant. They are not part of the shared signature.

## 3. Results of a full resize

```python
@dataclass
class CarveResult:
    image: np.ndarray  # resized image, same dtype and channels as the input
    seams: list[Seam]  # in removal order, each in the coordinates of the image at that step
    timings: dict[str, float]  # seconds: "energy", "find_seam", "remove", "total"
    frames: list[np.ndarray] | None  # optional intermediate images (visualisation / bonus 7a)
```

`carver.carve(image, target_height, target_width, finder, order="width_first", keep_frames=False) -> CarveResult`

- Raises `ValueError` if a target is larger than the current size, or less than 1.
- Recomputes the energy map before **every** seam (brief R1c).

## 4. Timing

Timings use `time.perf_counter()` around the finder call only, measured by the carver. This means the DP, greedy and graph cut are timed the same way, which matters for the Q3/Q4/Q5 comparisons.

## 5. Questions for reviewers

- HM / WN: is `find_seam(energy, orientation)` enough for your algorithm? Graph cut may prefer the image for forward energy. If so, we add an optional `image=` keyword rather than changing the shared signature.
- Should `Seam.indices` be `int64` or `intp`? Proposed: `int64` everywhere for predictable serialisation.
